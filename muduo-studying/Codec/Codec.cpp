#include "Codec.h"
#include <arpa/inet.h>
explicit Codec::Codec(const StringCallback& cb)
:_stringCallback(cb)
{}

void Codec::OnMessage(const Connptr& conn,Buffer * buf){
    //报文格式
    //4byte+message函数
    
    
    while (buf->readable()>=Header){
        //下面这部分还在学习中
        uint32_t be32=0;
        std::memcpy(&be32,buf->peek(),Header);
        uint32_t len=ntohl(be32);
        if (len>65535){
            std::cout<<"Invaild message length "<<len<<std::endl;
            break;
        }

        if (buf->readable()>Header+len){
            //半包
            break;
        }

        buf->retrieve(Header);
        std::string msg=buf->retrieveAsstring(len);

        if (_stringCallback){
            _stringCallback(conn,msg);
        }

    }
}

void Codec::send(const Connptr& conn,const std::string& msg){
    uint32_t len=static_cast<uint32_t>(msg.size());
    uint32_t be32=htonl(len);
    std::string packet;

    packet.resize(Header);

    std::memcpy(&packet[0],&be32,Header);

    packet+=msg;

    conn->send(packet);

}