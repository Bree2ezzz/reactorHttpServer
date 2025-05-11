//
// Created by 洛琪希 on 25-4-29.
//

#include "../headers/EpollDispatcher.h"
#include <sys/epoll.h>


EpollDispatcher::EpollDispatcher(std::shared_ptr<EventLoop> evLoop) : Dispatcher(evLoop)
{
    epfd_ = epoll_create(10);
    if (epfd_ == -1)
    {
        perror("epoll_create");
        exit(0);
    }
    events_ = std::make_unique<struct epoll_event[]>(maxNode_);

}

int EpollDispatcher::add()
{
    int ret = epollCtl(EPOLL_CTL_ADD);
    if(ret == -1)
    {
        perror("epollctl error");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::remove()
{
    int ret = epollCtl(EPOLL_CTL_DEL);
    if(ret == -1)
    {
        perror("epollctl error");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::modify()
{
    int ret = epollCtl(EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("epoll_crl modify");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::dispatch(int timeout)
{
    int count = epoll_wait(epfd_,events_.get(),maxNode_,timeout * 1000);
    for(int i = 0; i < count; ++i)
    {
        auto events = events_[i].events;
        socket_t fd = events_[i].data.fd;
        if(events & EPOLLERR || events & EPOLLHUP)
        {
            continue;
        }
        if (events & EPOLLIN)
        {
            evLoop_->eventActive(fd, (int)FDEvent::ReadEvent);
        }
        if (events & EPOLLOUT)
        {
            evLoop_->eventActive(fd, (int)FDEvent::WriteEvent);
        }
    }
    return 0;
}

int EpollDispatcher::epollCtl(int op)
{
    epoll_event ev{0};
    ev.data.fd = channel_->getSocket();
    int events = 0;
    if(channel_->getEvents() & static_cast<int>(FDEvent::ReadEvent))
    {
        events |= EPOLLIN;
    }
    if(channel_->getEvents() & static_cast<int>(FDEvent::WriteEvent))
    {
        events |= EPOLLOUT;
    }
    ev.events = events;
    int ret = epoll_ctl(epfd_,op,channel_->getSocket(),&ev);
    return ret;
}
