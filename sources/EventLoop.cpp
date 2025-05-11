//
// Created by 洛琪希 on 25-4-28.
//

#include "../headers/EventLoop.h"

#include "../headers/Dispatcher.h"
#include <cassert>
EventLoop::EventLoop() : EventLoop(std::string())
{

}

EventLoop::EventLoop(const std::string& threadName) : isQuit(true),wakeupFdRead_(-1),wakeupFdWrite_(-1)
{
    threadId_ = std::this_thread::get_id();
    // dispatcher_ = new EpollDispatcher;暂未实现
    threadName_ = threadName.empty() ? "MainThread" : threadName;
    channelMap_.clear();
    createSocketPair(wakeupFdRead_,wakeupFdWrite_);
}

bool EventLoop::createSocketPair(socket_t &readFd, socket_t &writeFd)
{
#ifdef _WIN32
    // Windows平台
    socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) {
        return false;
    }
    sockaddr_in addr{0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
    addr.sin_port = 0;//系统自动分配即可
    if(bind(listener,reinterpret_cast<sockaddr*>(&addr),sizeof(addr)) < 0)
    {
        closesocket(listener);
        return false;
    }
    if(listen(listener,128) < 0)
    {
        closesocket(listener);
        return false;
    }
    socket_t connector = socket(AF_INET, SOCK_STREAM, 0);
    if(connector == INVALID_SOCKET)
    {
        closesocket(listener);
        return false;
    }
    if(connect(connector,reinterpret_cast<sockaddr*>(&addr),sizeof(addr)) < 0)
    {
        closesocket(connector);
        closesocket(listener);
        return false;
    }
    socket_t acceptor = accept(listener, nullptr, nullptr);
    if(acceptor == INVALID_SOCKET)
    {
        closesocket(connector);
        closesocket(listener);
        return false;
    }
    closesocket(listener);
    readFd = acceptor;//自己的套接字，用于唤醒。这个放在检测集合中
    writeFd = connector;//这个用想acceptor随便写点来达成唤醒
    return true;


#else
    // Linux平台
    socket_t mysocket[2];
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, mysocket);
    if (ret == -1)
    {
        perror("socketpair");
        return false;
    }
    readFd = mysocket[0];
    writeFd = mysocket[1];
    return true;
#endif
}

int EventLoop::writeCallback()
{
    return 0;
}

int EventLoop::readCallback()
{
    char buf[4]{0};
    recv(wakeupFdRead_,buf,sizeof(buf),0);
    return 0;
}

int EventLoop::addTask(std::shared_ptr<Channel> channel, ElemType type)
{
    std::lock_guard<std::mutex>mtx(eventLoopMutex_);
    {
        std::shared_ptr<ChannelElement> element = std::make_shared<ChannelElement>();
        element->type = type;
        element->channel = channel;
        taskQueue_.emplace(element);
    }
    if(threadId_ == std::this_thread::get_id())
    {
        //处理任务
        processTaskQ();
    }
    else
    {
        //主线程派发任务 需要唤醒线程
        taskWakeUp();
    }
    return 0;
}

void EventLoop::initWakeupChannel()
{
    std::shared_ptr<Channel> wakeupChannel = std::make_shared<Channel>(wakeupFdRead_,FDEvent::ReadEvent,
        std::static_pointer_cast<ChannelContextBase>(shared_from_this()));
    addTask(wakeupChannel,ElemType::ETADD);
}

void EventLoop::taskWakeUp()
{
    //唤醒线程，随便发点数据
    send(wakeupFdWrite_,"a",1,0);
}

int EventLoop::processTaskQ()
{
    while(!taskQueue_.empty())
    {
        std::shared_ptr<ChannelElement> node;
        {
            std::lock_guard<std::mutex> mtx(eventLoopMutex_);
            node = taskQueue_.front();
            taskQueue_.pop();
        }
        std::shared_ptr<Channel> channel = node->channel;
        if(node->type == ElemType::ETADD)
        {
            addToMap(channel);
        }
        else if(node->type == ElemType::ETDELETE)
        {
            deleteFromMap(channel);
        }
        else if(node->type == ElemType::ETMODIFY)
        {
            modifyMap(channel);
        }
    }

    return 0;
}

int EventLoop::addToMap(std::shared_ptr<Channel> channel)
{
    socket_t fd = channel->getSocket();
    if(channelMap_.find(fd) == channelMap_.end())
    {
        channelMap_.emplace(std::pair(fd,channel));
        dispatcher_->setChannel(channel);
        int ret = dispatcher_->add();
        return ret;
    }
    return -1;
}

int EventLoop::deleteFromMap(std::shared_ptr<Channel> channel)
{
    socket_t fd = channel->getSocket();
    if(channelMap_.find(fd) == channelMap_.end())
    {
        return -1;
    }
    dispatcher_->setChannel(channel);
    return dispatcher_->remove();
}

int EventLoop::modifyMap(std::shared_ptr<Channel> channel)
{
    socket_t fd = channel->getSocket();
    if(channelMap_.find(fd) == channelMap_.end())
    {
        return -1;
    }
    dispatcher_->setChannel(channel);
    return dispatcher_->modify();
}

int EventLoop::run()
{
    isQuit = false;
    if(threadId_ != std::this_thread::get_id())
    {
        return -1;
    }
    while(!isQuit)
    {
        dispatcher_->dispatch(2);
        processTaskQ();
    }
    return 0;
}

int EventLoop::eventActive(socket_t fd, int event)
{
    if(fd < 0)
    {
        return -1;
    }
    std::shared_ptr<Channel> channel = channelMap_[fd];
    assert(channel->getSocket() == fd);
    if (event & (int)FDEvent::ReadEvent)
    {
        channel->getContext()->readCallback();
    }
    if (event & (int)FDEvent::WriteEvent)
    {
        channel->getContext()->writeCallback();
    }
    return 0;
}

std::thread::id EventLoop::getThreadId()
{
    return threadId_;
}
