# Muduo学习
**本学习笔记来源都可在 《Linux多线程服务器编程》陈硕著中找到**
## muduo buffer类的设计与使用
### 前情提要
muduo为什么要有buffer，其实还是为了解决tcp的粘包问题。
### buffer内部设计
buffer设计一块连续的内存，也就是一个vector，然后一个读指针，一个写指针。chensuo本人说
这个buffer设计有点类似于队列，队头读，队尾写。在哪用呢？在TcpConnection中有两个Buffer成员，一个inbuffer，一个outbuffer。
继续说，两个index把buffer分成了三个区域，prependable readable writable (待截图)
### buffer的设计思想
缓冲区的设计理念，希望是一次读取更多的数据，更大的缓冲区往内存更大，但是如果连接来的多了，会占用内存。muduo用readv(2)和栈巧妙解决了,大致思想就是同时借用内存和栈，在有限的内存中存，不够的话写入栈上。buffer的另外两个成员变量用的是size_t类型，防止的就是迭代器失效。
### buffer的操作
在buffer里面，基本有两个变量，一个负责初始化两个指针位置，一个负责初始化内存大小。写入数据之后，writerIndex_会向后移动。读取和写入其实就是一个双指针思想，保证read指针、、< write指针，有一定的情况下移动其中一个指针。使用vector其中一个好处就是其可以实现自增长。这就要提到vector的深拷贝与浅拷贝了。但是buffer有一个避免深拷贝的方法，如果prependable + writable的空间能够提供我们的写入需求的话，会把数据前移，把前后空间拼凑在一块。
### protobuf 的知识补充
这里贴一段介绍google protobuf buffers的简单介绍https://zhuanlan.zhihu.com/p/141415216。
在网络服务程序中，往往会以“消息”的单位进行通信(其实就是一种结构体)编解码器(codec)，其基本功能之一，就是实现TCP的分包，确定每条消息的长度。它在TcpConnection和Server之间，做到一个拦截数据包，解析出一个完整的消息对象再调用Server的处理函数。注意这个中间件会自己做buffer到string的转换，这部分详见书p230下面的介绍。
### 限制服务器最大连接数
多线程程序中使用muduo提供的AtomicInt32这个变量去检查，如果比这个大就可以丢掉连接。
## muduo计时器
藏在exampls/asio/tutorial里面，期中timer5是多线程中回调计时器。不仅如此，我们可以设计一个timing wheel去踢掉空闲连接，大致算法就是使用8个桶组成的循环队列。(此处待附图)，是时间轮旋转，然后只用检查一个桶。chenshuo给出的示范代码在idleconnection中。
## "串并转换"连接服务器
这是一个网游连接服务器的功能需求，这部分代码在multiplexer中有。因为网游服务器是一种IO密集型服务器，这个multiplexer把多个客户连接汇聚成一个内部的TCP连接，其实就是一种代理
## Reactor模式
Reactor模式就是事件分发机制，
### Channel class
每个Channel对象自始至终只在一个Eventloop当中，因此每个Channel对象都只属于某一个IO线程每个channle对象也只负责一个fd的IO事件分发。
## EventLoop
什么都不做的EventLoop。在one loop per thread中，每个线程只有一个Eventloop对象。