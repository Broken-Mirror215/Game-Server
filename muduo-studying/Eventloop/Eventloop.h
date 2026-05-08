#pragma once
#include <memory>
#include <vector>
#include "../Epoller/Epoller.h"
#include <functional>
#include <vector>
#include <atomic> //这是一个原子操作的头文件
#include <pthread.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <string.h>
class Epoller;
class Channel;
typedef std::vector<Channel*> Channellist;
class Eventloop
{
public:

    using functor=std::function<void()>;
    Eventloop();
    void loop();
    void quit();


    void updateChannel(Channel * c1);//事件注册
    void removeChannel(Channel* c1);//事件的删除

    void runinLoop(functor cb);
    void queueinLoop(functor cb);//把任务丢进pendingfunctors里面
    void doqueue();

    bool isInloopthead() const;
    bool assertInloopThead() const;
private:
    int createventfd();
    void wakeup();
    void handleRead();

private:
    bool _looping;
    std::atomic<bool> _quit;
    std::shared_ptr<Epoller > _epr;
    Channellist activechannels;
    
    //当前eventloop是哪个线程的
    pthread_t _threadid;

    //eventfd，这个效果是让别的线程去写他，然后唤醒这个线程的epoll_wait
    int _wakefd;
    std::unique_ptr<Channel> _wakeupChannel;

    //多线程
    pthread_mutex_t _mutex;
    std::vector<functor> _pendingfunctors;
    bool _callingfunctors;
};
