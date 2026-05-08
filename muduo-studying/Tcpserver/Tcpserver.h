#pragma once

#include "../Eventloop/Eventloop.h"
#include "../Acceptor/Acceptor.h"
#include "../Tcpconnection/Tcpconnection.h"
#include <unordered_map>
#include <memory>
#include <string>
#include "../EventloopThreadpool/EventloopThreadpool.h"
class Tcpserver
{
public:
    using TcpconnectionPtr = std::shared_ptr<Tcpconnection>;
    using MessageCallback=Tcpconnection::MessageCallback;
    using Connectionback=std::function<void(const TcpconnectionPtr&)>;
    using CloseConnback=std::function<void (const TcpconnectionPtr&)>;

    Tcpserver(Eventloop* loop, const char *ip, uint16_t port);
    void start();

    //这个是subloop的线程数量
    void setThreadNum(int num);

    void setmessageback(const MessageCallback& cb);

    void _setConnectionback(const Connectionback& cb);

    void _setCloseConnback(const CloseConnback& cb);

private:
    void newConnection(int connfd);
    void removeConnection(const TcpconnectionPtr & conn);

    void removeConnInLoop(const TcpconnectionPtr& conn);

private:
    Eventloop* _loop; //这是mainloop
    Acceptor _acceptor;

    std::unordered_map<int, TcpconnectionPtr> _conns;

    std::unique_ptr<EventloopThreadpool> _threadpool;
    int _threadnum;
    bool _started;

    MessageCallback _messageback;
    //晚点需要注册的消息回调
    Connectionback _connback;
    //晚点要注册的关闭回调
    CloseConnback _closeback;


    
};