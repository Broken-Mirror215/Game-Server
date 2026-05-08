#include "EventloopThread.h"

EventloopThread::EventloopThread()
:_loop(nullptr),
_started(false),
_existing(false)
{
    pthread_mutex_init(&_mutex,nullptr);
    pthread_cond_init(&_cond,nullptr);
}

Eventloop* EventloopThread::EventloopStart(){
    _started=true;
    int ret=pthread_create(&_threadid,nullptr,&StartThread,this);
    if (ret!=0){
        std::cerr<<"thread create error"<<std::endl;
        std::abort();
    }

    Eventloop* loop=nullptr;
    pthread_mutex_lock(&_mutex);
    while (_loop==nullptr){
        pthread_cond_wait(&_cond,&_mutex);
    }

    loop=_loop;
    pthread_mutex_unlock(&_mutex);

    return loop;

}

void* EventloopThread::StartThread(void * arg){
    EventloopThread* thread=static_cast<EventloopThread*>(arg);
    thread->Threadfunc();
    return nullptr;
}

void EventloopThread::Threadfunc(){
    Eventloop loop;
    pthread_mutex_lock(&_mutex);
    _loop=&loop;

    pthread_cond_signal(&_cond);
    pthread_mutex_unlock(&_mutex);
    loop.loop();
    pthread_mutex_lock(&_mutex);
    _loop=nullptr;
    pthread_mutex_unlock(&_mutex);
}