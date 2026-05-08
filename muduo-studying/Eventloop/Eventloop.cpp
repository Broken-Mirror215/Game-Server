#include "Eventloop.h"
#include "../Channel/Channel.h"
Eventloop::Eventloop()    
:_looping(false),
_quit(false),
_epr(new Epoller(this)),
_threadid(pthread_self()),
_wakefd(createventfd()),
_wakeupChannel(std::make_unique<Channel>(this,_wakefd)),
_callingfunctors(false)
{
    if (pthread_mutex_init(&_mutex,nullptr)!=0){
        std::cerr<<"the mutex init failed"<<std::endl;
        std::abort();
    }

    //这个感觉不对
    _wakeupChannel->SetReadCallBack(
        std::bind(&Eventloop::handleRead,this)
    );

    _wakeupChannel->enableReading();//更新内核
}

void Eventloop::loop(){
    _looping =true;

    while (!_quit){
        activechannels.clear();
        //阻塞,事件注册进activechannels里面
        _epr->Epoll(10000,activechannels);
        for (auto C:activechannels){
            C->handlerEvent();
        }
        //把队列的东西放进来执行清除了
        doqueue();
    }
    _looping=false;
}

void Eventloop::quit(){
    _quit=true;

    //如果不是本loop的线程用quit了
    //就唤醒epoll_wait，让这个loop赶紧退出。
    if (isInloopthead()){
        wakeup();
    }
}

void Eventloop::updateChannel(Channel * c1){
    _epr->updateChannel(c1);
}

void Eventloop::removeChannel(Channel* c1){
    _epr->removeChannel(c1);
}

void Eventloop::queueinLoop(functor cb){
    pthread_mutex_lock(&_mutex);
    _pendingfunctors.push_back(cb); 
    pthread_mutex_unlock(&_mutex);
    
    
     /*
        其他线程投递任务时，要唤醒 epoll_wait。
        例如：
        subLoop 正在 epoll_wait 睡眠
        mainLoop 往 subLoop 投递一个任务
        如果不 wakeup，subLoop 可能一直睡着，不会立刻执行任务。
    */
   
    if (!isInloopthead() || _callingfunctors)
    {
        wakeup();
    }
}

void Eventloop::doqueue(){
    std::vector<functor> functors;
    _callingfunctors=true;

    pthread_mutex_lock(&_mutex);
    functors.swap(_pendingfunctors);
    pthread_mutex_unlock(&_mutex);

    for (const functor& functor : functors)
    {
        functor();
    }
    
    _callingfunctors=false;
}

bool Eventloop::isInloopthead() const
{
    return pthread_equal(_threadid, pthread_self());
}

bool Eventloop::assertInloopThead() const{
    if (!isInloopthead()){
        std::cerr<<"eventloop in the wrong thread"<<std::endl;
        std::abort();
    }
    return true;
}

void Eventloop::runinLoop(functor cb){
    if (isInloopthead()){
        cb();
    }
    else
    {
        queueinLoop(std::move(cb));
    }
}


int Eventloop::createventfd(){
    //内核的事件计数fd
    int fd=::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);

    if (fd < 0)
    {
        std::cerr << "eventfd create error: " << strerror(errno) << std::endl;
        std::abort();
    }

    return fd;
}

void Eventloop::wakeup()
{
    uint64_t one = 1;
    //这个相当于把内核事件记录fd置1
    ssize_t n = ::write(_wakefd, &one, sizeof(one));

    if (n != sizeof(one))
    {
        std::cerr << "Eventloop::wakeup write error: "<< strerror(errno) << std::endl;
    }
}

void Eventloop::handleRead()
{
    uint64_t one = 1;

    ssize_t n = ::read(_wakefd, &one, sizeof(one));

    if (n != sizeof(one))
    {
        std::cerr << "Eventloop::handleRead read error: " << strerror(errno) << std::endl;
    }
}



