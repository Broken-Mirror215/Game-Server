#include "Epoller.h"
#include <cerrno>
#include <iostream>
Epoller::Epoller(Eventloop * looper)
:_looper(looper),
_epollfd(::epoll_create(EPOLL_CLOEXEC)),
_events(1024)
{}

void Epoller::Epoll(int timeoutMs,Channellist& activeChannels){
    int numEvents = ::epoll_wait(_epollfd, _events.data(),static_cast<int>(_events.size()),timeoutMs);

    if (numEvents > 0)
    {
        fillActiveChannels(numEvents, activeChannels);//调用内核注册

        //扩容，这里后面改一个深拷贝扩容
        if (numEvents == static_cast<int>(_events.size()))
        {
            _events.resize(_events.size() * 2);
        }
    }
}
void Epoller::fillActiveChannels(int numEvents,Channellist& activechannels){
    for (int i=0;i<numEvents;i++){
        Channel * channel=static_cast<Channel *>(_events[i].data.ptr);
        channel->set_revents(_events[i].events);
        activechannels.push_back(channel);
    }
}

void  Epoller::updateChannel(Channel * c1){
    int fd=c1->fd();
    struct epoll_event ev;
    ev.data.ptr=c1;
    ev.events=c1->events();

    if (_channels.find(fd) == _channels.end())
    {
        if (::epoll_ctl(_epollfd, EPOLL_CTL_ADD, fd, &ev) < 0)
        {
            perror("epoll_ctl add error\n");
        }
        _channels[fd] =c1;
    }
    else
    {
        if (::epoll_ctl(_epollfd, EPOLL_CTL_MOD, fd, &ev) < 0)
        {
            std::cerr << "epoll_ctl mod error\n";
            std::abort();
        }
    }
}