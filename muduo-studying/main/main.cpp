#include <iostream>
#include "../Eventloop/Eventloop.h"
#include "../Channel/Channel.h"
#include <unistd.h>
#include "../Acceptor/Acceptor.h"
#include "../Tcpconnection/Tcpconnection.h"
#include <unordered_map>
#include <memory>
#include "../Tcpserver/Tcpserver.h"
#include "../Buffer/buffer.h"
#include "../Game-Server/Game-server.h"
#include "../Codec/Codec.h"
int main(){
    Eventloop loop;
    //server最先拿到了mainloop，然后传到了channel和_acceptor
    Tcpserver server(&loop,"127.0.0.1",8080);
    gServer GameServer;
    server.setThreadNum(10);
    //下面codec在用的时候会直接顺带用了这个
    //这个codec还是用在消息层的回调
    Codec codec([&GameServer](const std::shared_ptr<Tcpconnection> & conn,const std::string&msg) 
    {
        std::cout<<"business msg "<<msg<<std::endl;
        GameServer.onMessage(conn,msg);
    });

    //这个server用了tcpconnection里面的函数体。server先设置回调，然后再把server的回调给tcpconnection
    server.setmessageback([&codec](const std::shared_ptr<Tcpconnection>& conn,Buffer* buf){
        codec.OnMessage(conn,buf);
    });

    server._setConnectionback([&GameServer](const std::shared_ptr<Tcpconnection>& conn){
        GameServer.onConnect(conn);
    });

    server._setCloseConnback([&GameServer](const std::shared_ptr<Tcpconnection>& conn){
        GameServer.onDisconnect(conn);
    });
    server.start();
    loop.loop();
}
