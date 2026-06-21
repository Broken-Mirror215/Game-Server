#pragma once
#include <functional>
class Eventloop;

using EventCallback=std::function<void()>;
class Channel
{
public:

    Channel(Eventloop * looper,int fd);

    void handlerEvent();
    //下面如果是写，读，链接事件，我先设置一个函数，等你链接就是这个的时候再处理。
    void SetReadCallBack(EventCallback cb) {readCallback_=std::move(cb);}
    void SetWriteCallBack(EventCallback cb) {writeCallback_=std::move(cb);}

    int fd() const { return _fd; }
    int events() const { return _events; }
    //卧槽，这里修改了revents
    void set_revents(int revt) { _revents= revt; }

    //这个是链条调用开始，设计的是fd->channel->evnetpoller->poller这个链
    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void Noneall();

    //event的掩码
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
private:
    void update();
private:
    int _fd;
    Eventloop *_looper;
    int _events;//这里用来统计事件的
    int _revents;//这里是内核里面实际告诉我发生了什么！

    EventCallback readCallback_;
    EventCallback writeCallback_;
};
