//
// Created by 洛琪希 on 25-4-28.
//

#ifndef DISPATCHER_H
#define DISPATCHER_H
#include <memory>
#include "Channel.h"
#include "EventLoop.h"
class EventLoop;
class Dispatcher {
public:
    explicit Dispatcher(std::shared_ptr<EventLoop> evLoop):evLoop_(evLoop){}
    virtual ~Dispatcher() = default;
    void setChannel(std::shared_ptr<Channel> channel);
    virtual int add() = 0;
    virtual int remove() = 0;
    virtual int modify() = 0;
    virtual int dispatch(int timeout) = 0;

    /*channel是封装文件描述符，evLoop是一个事件循环，channel每次调用时可能是不一样的，evLoop是专属于某个线程的，不会改变
     */
    std::shared_ptr<Channel> channel_;
    std::shared_ptr<EventLoop> evLoop_;
};



#endif //DISPATCHER_H
