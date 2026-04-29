#pragma once
#include <memory>
#include "../Tcpconnection/Tcpconnection.h"
#include "../Buffer/buffer.h"
#include <cstring>
class Codec{
public:
    using Connptr=std::shared_ptr<Tcpconnection>;
    using StringCallback=std::function<void(const Connptr&,const std::string&)>;

    explicit Codec(const StringCallback& cb);

    void OnMessage(const Connptr&,Buffer * buf);

    static void send(const Connptr&,const std::string& msg);

private:
    static const int Header=4;
private:
    StringCallback _stringCallback;
};  