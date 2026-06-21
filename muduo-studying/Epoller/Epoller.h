#pragma once
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

class Channel;
class Eventloop;

using Channellist=std::vector<Channel*>;
class Epoller{
public:

    Epoller(Eventloop * looper);
    ~Epoller();

    void Epoll(int timeoutMs,Channellist& activechannels);
    void updateChannel(Channel * c1);
private:
    void fillActiveChannels(int numEvents,Channellist& activechannels);

private:
    Eventloop* _looper;
    int _epollfd;
    std::vector<struct epoll_event> _events;//这是一个事件集合。
    std::unordered_map<int,Channel*> _channels;

};
