#pragma once
#include "../Eventloop/Eventloop.h"
#include "../EventloopThread/EventloopThread.h"
#include <memory>
#include <vector>
class EventloopThreadpool{
public:
    EventloopThreadpool(Eventloop*,int num);

    void Start();

    Eventloop* getnextloop();


private:
    Eventloop * _mainloop;
    int _threadnum;
    int _next;



    std::vector<std::unique_ptr<EventloopThread>> _threads;
    std::vector<Eventloop*> _loops;
};