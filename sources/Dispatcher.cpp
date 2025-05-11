//
// Created by 洛琪希 on 25-4-28.
//

#include <utility>

#include "../headers/Dispatcher.h"



void Dispatcher::setChannel(std::shared_ptr<Channel> channel)
{
    channel_ = channel;
}
