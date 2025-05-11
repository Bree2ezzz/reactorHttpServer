//
// Created by 洛琪希 on 25-4-28.
//

#ifndef EVENTLOOP_H
#define EVENTLOOP_H
#include <thread>
#include <queue>
#include <map>
#include "Channel.h"
#include <mutex>
#include <string>
enum class ElemType:char{ETADD,ETDELETE, ETMODIFY};

struct ChannelElement
{
    ElemType type;   // 如何处理该节点中的channel
    std::shared_ptr<Channel> channel;
};
class Dispatcher;

class EventLoop : public ChannelContextBase , public std::enable_shared_from_this<EventLoop>
{
public:
    EventLoop();
    EventLoop(const std::string& threadName);
    ~EventLoop() = default;
    bool createSocketPair(socket_t& readFd,socket_t& writeFD);
    int writeCallback() override;
    int readCallback() override;
    int addTask(std::shared_ptr<Channel> channel,ElemType type);
    void initWakeupChannel();
    int processTaskQ();

    int addToMap(std::shared_ptr<Channel> channel);
    int deleteFromMap(std::shared_ptr<Channel> channel);
    int modifyMap(std::shared_ptr<Channel> channel);

    int run();
    int eventActive(socket_t fd, int event);//处理激活的文件描述符
    std::thread::id getThreadId();


private:
    void taskWakeUp();
    bool isQuit;
    std::shared_ptr<Dispatcher> dispatcher_;
    std::queue<std::shared_ptr<ChannelElement>> taskQueue_;
    std::map<socket_t,std::shared_ptr<Channel>> channelMap_;
    std::string threadName_;
    std::thread::id threadId_;
    std::mutex eventLoopMutex_;

    socket_t wakeupFdRead_;   // 读端，注册到epoll
    socket_t wakeupFdWrite_;  // 写端，发送唤醒信号
};



#endif //EVENTLOOP_H
