#pragma once
#include "../Eventloop/Eventloop.h"
#include "../Channel/Channel.h"
#include <functional>
#include <memory>
#include <string>
#include "../Buffer/buffer.h"
#include <cerrno>
#include <unistd.h>
class Tcpconnection:public std::enable_shared_from_this<Tcpconnection>
{
public:
    using Connptr=std::shared_ptr<Tcpconnection>;
    using CloseCallback= std::function<void(const Connptr &)>;//不用fd是因为业务层不好修改了。
    using MessageCallback=std::function<void(const Connptr&,Buffer*)>;
    Tcpconnection(Eventloop* loop,int connfd);
    

    void connestablished();
    void conndestroyed();
    void setcloseCallback(CloseCallback cb){
        _closeback=std::move(cb);
    }

    void setmessageback(MessageCallback cb);

    void send(const std::string& msg);

    int fd() const;

    Eventloop* getloop() const;

private:
    void handleread();
    void handleclose();

    void sendInloop(const std::string& msg); 

private:
    Eventloop *_loop;
    int _connfd;
    Channel _channel;

    CloseCallback _closeback;
    MessageCallback _messageback;
    Buffer _inputbuffer;
};
