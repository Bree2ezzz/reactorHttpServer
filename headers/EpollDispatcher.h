//
// Created by 洛琪希 on 25-4-29.
//

#ifndef EPOLLDISPATCHER_H
#define EPOLLDISPATCHER_H

#include <sys/epoll.h>

#include "Dispatcher.h"
#include "Channel.h"
#include "EventLoop.h"

class EpollDispatcher : public Dispatcher
{
public:
    EpollDispatcher(std::shared_ptr<EventLoop> evLoop);
    ~EpollDispatcher() = default;
    int add() override;
    int remove() override;
    int modify() override;
    int dispatch(int timeout) override ;

private:
    int epollCtl(int op);

    socket_t epfd_;
    const int maxNode_ = 1024;
    std::unique_ptr<struct epoll_event[]> events_;


};



#endif //EPOLLDISPATCHER_H
