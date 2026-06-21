#include <iostream>
#include "../Eventloop/Eventloop.h"
#include <unistd.h>
int main(){
    Eventloop loop;
    Channel ch(&loop,0);

    //这是给ch注册回调函数.....
    ch.SetReadCallBack([&loop]()
    {
        char buffer[1024];
        ssize_t n = ::read(0, buffer, sizeof(buffer) - 1);
    if (n > 0)
    {
        std::string msg(buffer, n);
        std::cout << "echo from server" << msg;

        if (msg.find("quit") != std::string::npos)
        {
            loop.quit();
        }
    }
    });

    ch.enableReading();
    //上面这层就已经注册好读事件了
    loop.loop();
}