#include "Eventloop.h"

Eventloop::Eventloop()    
:_looping(false),
_quit(false),
_epr(new Epoller(this))
{}

void Eventloop::loop(){
    _looping =true;

    while (!_quit){
        activechannels.clear();
        _epr->Epoll(10000,activechannels);
        for (auto C:activechannels){
            C->handlerEvent();
        }
    }
    _looping=false;

}

void Eventloop::quit(){
    _quit=true;
}

void Eventloop::updateChannel(Channel * c1){
    _epr->updateChannel(c1);
}


