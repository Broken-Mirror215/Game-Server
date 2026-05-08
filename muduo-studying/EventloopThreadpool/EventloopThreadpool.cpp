#include "EventloopThreadpool.h"

EventloopThreadpool::EventloopThreadpool(Eventloop* mainloop,int num)
:_mainloop(mainloop),
_threadnum(num),
_next(0)
{}

void EventloopThreadpool::Start(){

    for (int i=0;i<_threadnum;i++){
        std::unique_ptr<EventloopThread> t (new EventloopThread());

        Eventloop *loop=t->EventloopStart();

        _threads.push_back(std::move(t));
        _loops.push_back(loop);
    }
}

Eventloop* EventloopThreadpool::getnextloop(){
    Eventloop * loop=_mainloop;
    if (!_loops.empty()){
        loop=_loops[_next];
        _next++;

        if (_next>=static_cast<int>(_loops.size())){
            _next=0;
        }
    }

    return loop;
}

