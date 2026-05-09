# Reactor 调用链路详解

这份文档用于拆解项目中的核心调用链路。它和根目录 `README.md` 的关系是：

```text
README.md
  用来快速了解项目和阅读路线。

docs/reactor-flows.md
  用来深入理解各条调用链路。
```

## 从 main 出发的启动链路

程序从 `main/main.cpp` 开始：

```cpp
Eventloop loop;
Tcpserver server(&loop, "127.0.0.1", 8080);
gServer GameServer;
server.setThreadNum(10);
```

这里创建了主线程的 `mainLoop`，并把它交给 `Tcpserver`。

随后注册业务回调：

```text
连接建立 -> GameServer.onConnect(conn)
消息到达 -> Codec.OnMessage(conn, buf) -> GameServer.onMessage(conn, msg)
连接关闭 -> GameServer.onDisconnect(conn)
```

最后启动：

```cpp
server.start();
loop.loop();
```

`server.start()` 启动线程池并注册监听 fd；`loop.loop()` 让主线程进入 Reactor 事件循环。

## Eventloop / Thread / Threadpool 的关系

`Eventloop` 是 Reactor 核心，负责：

```text
epoll_wait()
处理 activeChannels
执行 pending functors
```

`EventloopThread` 负责创建一个子线程，并在子线程里创建一个 `Eventloop`：

```text
EventloopThread::EventloopStart()
-> pthread_create()
-> EventloopThread::StartThread()
-> EventloopThread::Threadfunc()
-> Eventloop loop
-> loop.loop()
```

`EventloopThreadpool` 负责创建多个 `EventloopThread`：

```text
EventloopThreadpool::Start()
-> 创建 N 个 EventloopThread
-> 每个线程中创建一个 subLoop
-> 保存所有 subLoop 指针
```

新连接到来后，`Tcpserver` 会轮询选择一个 subLoop：

```text
Eventloop* ioloop = _threadpool->getnextloop();
```

然后这个连接后续的 IO 事件就由该 subLoop 处理。

## 链路一：新连接到来时的链路

监听 fd 由 `Acceptor` 管理，并且挂在主线程的 `mainLoop` 上。

启动时：

```text
main
-> Tcpserver server(&mainLoop)
-> Tcpserver 内部创建 Acceptor(&mainLoop)
-> Acceptor 内部创建 acceptChannel(mainLoop, listenfd)
```

`Acceptor` 构造时设置监听 fd 的读回调：

```cpp
acceptChannel.SetReadCallBack([this]() {
    handleRead();
});
```

当客户端连接到来：

```text
客户端 connect
-> listenfd 变成可读
-> mainLoop 的 epoll_wait 返回 acceptChannel
-> Channel::handlerEvent()
-> acceptChannel 的 readCallback_
-> Acceptor::handleRead()
-> accept() 得到 connfd
```

`Acceptor` 不直接管理连接，而是通过回调通知 `Tcpserver`：

```text
Acceptor::handleRead()
-> _ConnReadback(connfd)
-> Tcpserver::newConnection(connfd)
```

`Tcpserver::newConnection()` 做几件事：

```text
1. 从 EventloopThreadpool 选择一个 subLoop
2. 创建 Tcpconnection(ioloop, connfd)
3. 保存到 _conns 连接表
4. 给 Tcpconnection 设置 message callback 和 close callback
5. 把 conn->connestablished() 投递给 subLoop
```

链路：

```text
Tcpserver::newConnection(connfd)
-> EventloopThreadpool::getnextloop()
-> Tcpconnection(ioloop, connfd)
-> ioloop->runinLoop(conn->connestablished)
-> subLoop 被 eventfd 唤醒
-> connfd 注册进 subLoop 的 epoll
```

从此以后，这个 `connfd` 的读写事件由对应的 subLoop 处理。

## 链路二：事件注册进内核的链路

fd 注册到 epoll 主要通过：

```text
Channel -> Eventloop -> Epoller -> epoll_ctl
```

### 监听 fd 注册

监听 fd 在 `Acceptor::listen()` 中注册读事件：

```text
Acceptor::listen()
-> acceptChannel.enableReading()
-> Channel::update()
-> Eventloop::updateChannel(this)
-> Epoller::updateChannel(channel)
-> epoll_ctl(EPOLL_CTL_ADD/MOD)
```

### 连接 fd 注册

新连接建立后，`Tcpconnection::connestablished()` 注册连接 fd 的读事件：

```text
Tcpconnection::connestablished()
-> _channel.enableReading()
-> Channel::update()
-> Eventloop::updateChannel(this)
-> Epoller::updateChannel(channel)
-> epoll_ctl(EPOLL_CTL_ADD/MOD)
```

### eventfd 注册

每个 `Eventloop` 构造时都会创建一个 `eventfd`，用于跨线程唤醒：

```text
Eventloop::Eventloop()
-> createventfd()
-> _wakeupChannel = Channel(this, _wakefd)
-> _wakeupChannel->SetReadCallBack(Eventloop::handleRead)
-> _wakeupChannel->enableReading()
-> epoll_ctl()
```

所以每个 Eventloop 的 epoll 中至少会有一个用于唤醒的 `eventfd`。

## 链路三：事件从内核中被读取然后处理的链路

所有 IO 事件都从 `Eventloop::loop()` 开始处理：

```text
Eventloop::loop()
-> Epoller::Epoll(timeout, activechannels)
-> epoll_wait()
-> Epoller::fillActiveChannels()
-> activechannels.push_back(channel)
```

`epoll_wait()` 返回的是内核检测到的活跃事件，`Epoller` 会把这些事件重新映射回 `Channel*`。

然后 `Eventloop` 遍历所有活跃 Channel：

```text
for channel in activechannels:
    channel->handlerEvent()
```

`Channel::handlerEvent()` 根据发生的事件调用对应回调：

```text
EPOLLIN  -> readCallback_()
EPOLLOUT -> writeCallback_()
```

不同 fd 的读事件含义不同：

```text
listenfd 可读
-> 有新连接
-> Acceptor::handleRead()
-> accept()

connfd 可读
-> 客户端发来了数据或关闭连接
-> Tcpconnection::handleread()
-> read()

eventfd 可读
-> 其他线程投递了任务
-> Eventloop::handleRead()
-> doqueue()
```

因此，`Channel` 本身不关心业务，它只负责：

```text
fd 发生事件 -> 调用之前绑定好的回调函数
```

## 跨线程任务投递链路

当主线程需要让某个 subLoop 执行任务时，会调用：

```cpp
ioloop->runinLoop(...)
```

如果当前线程不是该 `ioloop` 所属线程：

```text
Eventloop::runinLoop(cb)
-> Eventloop::queueinLoop(cb)
-> 把 cb 放进 _pendingfunctors
-> wakeup()
-> write(_wakefd)
```

`_wakefd` 是 `eventfd`，被写入后会变成可读，从而唤醒阻塞在 `epoll_wait()` 的 subLoop。

subLoop 线程被唤醒后：

```text
epoll_wait 返回
-> wakeupChannel 触发 readCallback
-> Eventloop::handleRead()
-> 读掉 eventfd 中的计数
-> Eventloop::doqueue()
-> 执行 pending functors
```

这个机制用于：

```text
主线程把 conn->connestablished() 投递给 subLoop
其他线程把 sendInloop() 投递给连接所属 subLoop
关闭连接时把 conndestroyed() 投递给连接所属 subLoop
```

## 链路四：业务消息转发链路

消息链路从 `main.cpp` 注册回调开始。

第一层：`Tcpconnection` 读到原始字节后，交给 `Codec`：

```cpp
server.setmessageback([&codec](const std::shared_ptr<Tcpconnection>& conn, Buffer* buf) {
    codec.OnMessage(conn, buf);
});
```

第二层：`Codec` 拆出完整消息后，交给 `GameServer`：

```cpp
Codec codec([&GameServer](const std::shared_ptr<Tcpconnection>& conn,
                          const std::string& msg) {
    GameServer.onMessage(conn, msg);
});
```

新连接创建时，`Tcpserver` 把消息回调装进每个 `Tcpconnection`：

```text
Tcpserver::newConnection()
-> conn->setmessageback(_messageback)
```

客户端发送消息后：

```text
subLoop epoll_wait 返回 connfd
-> Channel::handlerEvent()
-> Tcpconnection::handleread()
-> read(_connfd)
-> _inputbuffer.Append()
-> _messageback(conn, &_inputbuffer)
-> Codec::OnMessage(conn, buf)
```

`Codec` 使用协议格式：

```text
4 字节大端长度头 + 消息体
```

当 Buffer 中有完整消息时：

```text
Codec::OnMessage()
-> 读取 4 字节长度
-> 判断是否收到完整 body
-> 取出 string msg
-> _stringCallback(conn, msg)
-> GameServer::onMessage(conn, msg)
```

业务层处理：

```text
msg == "MATCH"
-> gServer::handlematch()
-> 进入匹配队列
-> 两个玩家匹配成功后创建 Room
-> 通知双方 match success

其他消息
-> gServer::handleroommsg()
-> 找到玩家所在 Room
-> Room::Forward()
-> 转发给另一个玩家
```

完整链路：

```text
客户端发送数据
-> subLoop 发现 connfd 可读
-> Tcpconnection::handleread()
-> Buffer
-> Codec::OnMessage()
-> GameServer::onMessage()
-> MATCH / room message
-> Room::Forward()
-> Tcpconnection::send()
-> 所属 subLoop 执行 sendInloop()
-> write(connfd)
```

## 连接关闭链路

当客户端关闭连接，`read()` 返回 0：

```text
Tcpconnection::handleread()
-> read() == 0
-> Tcpconnection::handleclose()
-> close callback
-> Tcpserver::removeConnection(conn)
```

`Tcpserver` 会回到 mainLoop 删除连接表：

```text
Tcpserver::removeConnection()
-> mainLoop->runinLoop(removeConnInLoop)
-> _conns.erase(fd)
```

然后回到连接所属 subLoop 做连接销毁：

```text
conn->getloop()
-> ioloop->runinLoop(conn->conndestroyed)
-> Tcpconnection 从 epoll 删除 fd
-> close(fd)
```

## 小结

这个项目最值得复盘的点：

```text
1. Channel 只是 fd 和回调的绑定，不直接关心业务。
2. Eventloop 是事件分发器，负责 epoll_wait 和回调执行。
3. Acceptor 的读事件代表有新连接，可以 accept。
4. Tcpconnection 的读事件代表连接上有数据，可以 read。
5. Tcpserver 是网络层总控，负责连接创建、分发和销毁。
6. Codec 负责从 TCP 字节流中拆出完整业务消息。
7. GameServer 是业务层，负责玩家、匹配、房间和转发。
8. mainLoop 负责 accept，subLoop 负责连接 IO。
9. eventfd 用于跨线程唤醒 Eventloop。
10. one thread per loop 不是一个连接一个线程，而是一个 Eventloop 一个线程。
```
