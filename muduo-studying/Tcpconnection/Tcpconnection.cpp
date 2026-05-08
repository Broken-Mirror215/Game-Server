#include "Tcpconnection.h"

Tcpconnection::Tcpconnection(Eventloop* loop,int connfd)
:_loop(loop),
_connfd(connfd),
_channel(loop,connfd)//这里不直接传channel的原因是防止拷贝，这样多个fd表被拷贝走了。
{
    _channel.SetReadCallBack([this](){
        handleread();//塞进channel的readback里面。
    });
}


void Tcpconnection::connestablished(){
    _channel.enableReading();
}

void Tcpconnection::setmessageback(MessageCallback cb){
    _messageback = std::move(cb);
}

void Tcpconnection::handleread(){
    _loop->assertInloopThead();
    char buffer[1024];
    int n=::read(_connfd,buffer,sizeof(buffer)-1);
    if (n > 0)
    {
        _inputbuffer.Append(buffer,n);
        // std::cout << "client says: " << msg << std::flush;
        // if (msg.find("quit") != std::string::npos)
        // {
        //     _loop->quit();
        // }
        if (_messageback)
        {
            
            _messageback(shared_from_this(),&_inputbuffer);//这个是AI写的，我有点没看懂...
        }

    }
    else if (n == 0)
    {
        std::cout << "client closed fd = " << _connfd << std::endl;
        handleclose();
    }
    else
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK){
            return;
        }
        std::cout << "read error fd = " << _connfd << std::endl;
        handleclose();
    }
}

void Tcpconnection::handleclose(){
    _loop->assertInloopThead();
    if (_connfd<0){
        return ;
    }
    std::cout<<"Tcpconn handle close fd"<<_connfd<<std::endl;
    int fd=_connfd;
    //先清除关心的事件
    _channel.Noneall();
    auto self=shared_from_this();
    //关闭fd
    ::close(_connfd);
    if (_closeback){
        _closeback(shared_from_this());
    }
    else{
        conndestroyed();
    }
}

void Tcpconnection::send(const std::string& msg){
    if (_connfd<0){
        return;
    }

    auto self=shared_from_this();
    if (_loop->isInloopthead()){
        sendInloop(msg);
    }
    else{
        _loop->queueinLoop([self,msg](){
            self->sendInloop(msg);
        });
    }


}

void Tcpconnection::sendInloop(const std::string & msg){
    _loop->assertInloopThead();
    if (_connfd<0){
        return;
    }
    const char* data=msg.data();
    size_t left=msg.size();
    while (left>0){
        ssize_t n=::write(_connfd,data,left);
        if (n>0){
            data+=n;
            left-=n;
        }
        else if (n<0){
            if (errno==EINTR){
                continue;
            }

            if (errno==EAGAIN|| errno==EWOULDBLOCK){
                std::cout<<"send woubld block fd is "<<_connfd<<std::endl;
                break;
            }
            std::cout<<"error fd"<<_connfd<<std::endl;
            break;
        }
    }
}

int Tcpconnection::fd() const 
{
    return _connfd;
}

void Tcpconnection::conndestroyed(){
    _loop->assertInloopThead();
    if (_connfd<0){
        return ;
    }
    //这里是取消关心的事件
    _channel.Noneall();
    //从活跃channel里面删除
    _channel.removeChannel();
    ::close(_connfd);
    _connfd=-1;
}

Eventloop * Tcpconnection::getloop() const{
    return _loop;
}


