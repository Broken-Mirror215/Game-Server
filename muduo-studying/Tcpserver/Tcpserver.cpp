#include "Tcpserver.h"
#include "../Socket/Socket.h"

#include <iostream>

Tcpserver::Tcpserver(Eventloop* loop, const char*ip, uint16_t port)
:_loop(loop),
_acceptor(loop, ip, port),
_started(false),
_threadnum(0)
{
    _acceptor.setconnReadback([this](int connfd) {
        this->newConnection(connfd);
    });
}

void Tcpserver::newConnection(int connfd)
{
    _loop->assertInloopThead();
    if (connfd < 0)
    {
        std::cout << "accept error" << std::endl;
        return;
    }

    Socket::Setnonblock(connfd);

    //新的连接给某个subloop;
    Eventloop* ioloop=_threadpool->getnextloop();

    auto conn = std::make_shared<Tcpconnection>(ioloop, connfd);

    _conns[connfd] = conn;

    conn->setcloseCallback([this](const TcpconnectionPtr& conn) {
        if (_closeback){
            _closeback(conn);
        }
        this->removeConnection(conn);
    });

    conn->setmessageback(_messageback);//这是tcpserver把自己的_messageback转给了tcpconn的_messageback
    ioloop->runinLoop([conn](){
        conn->connestablished();
    });

    if (_connback)
    {
        _connback(conn);
    }
    std::cout << "new connection fd = " << connfd << std::endl;
}

void Tcpserver::removeConnection(const TcpconnectionPtr & conn)
{
    _loop->runinLoop([this,conn]{
        this->removeConnInLoop(conn);
    });
}

void Tcpserver::setmessageback(const MessageCallback& cb){
    _messageback=std::move(cb);
}

void Tcpserver::_setConnectionback(const Connectionback& cb){
    _connback=std::move(cb);
}

void Tcpserver::_setCloseConnback(const CloseConnback&cb){
    _closeback=std::move(cb);
}

void Tcpserver::setThreadNum(int num){
    _threadnum=num;
}

void Tcpserver::start(){
    if (_started){
        return;
    }
    _started=true;

    _threadpool.reset(new EventloopThreadpool(_loop,_threadnum));
    _threadpool->Start();

    _loop->runinLoop([this](){
        _acceptor.listen();
    });
}

void Tcpserver::removeConnInLoop(const TcpconnectionPtr& conn){
    _loop->assertInloopThead();
    int fd=conn->fd();
    std::cout<<"remove conn fd is "<<fd<<std::endl;

    auto it=_conns.find(fd);
    if (it!=_conns.end()&&it->second==conn){
        _conns.erase(it);
    }

    Eventloop * ioloop=conn->getloop();
    ioloop->runinLoop([this,conn](){
        conn->conndestroyed();
    });
}

