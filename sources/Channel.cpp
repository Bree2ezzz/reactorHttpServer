//
// Created by 洛琪希 on 25-4-28.
//

#include "../headers/Channel.h"

void Channel::writeEventEnable(bool flag)
{
    if(flag)
    {
        events_ |= static_cast<int>(FDEvent::WriteEvent);
    }
    else
    {
        events_ = events_ & ~static_cast<int>(FDEvent::WriteEvent);
    }

}

bool Channel::isWriteEventEnable() const
{
    return events_ & static_cast<int>(FDEvent::WriteEvent);
}

socket_t Channel::getSocket() const
{
    return fd_;
}

int Channel::getEvents() const
{
    return events_;
}

std::shared_ptr<ChannelContextBase> Channel::getContext()
{
    return context_.lock();
}
