//
// Created by 洛琪希 on 25-4-28.
//

#ifndef CHANNEL_H
#define CHANNEL_H
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include <memory>
using socket_t =
#ifdef _WIN32
    SOCKET;
#else
    int;
#endif
class ChannelContextBase
{
public:
    virtual int readCallback(){return 0;}
    virtual int writeCallback(){return 0;}

    ChannelContextBase() = default;
    virtual ~ChannelContextBase() = default;
};
enum class FDEvent
{
    TimeOut = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04
};
class Channel {

public:
    ~Channel() = default;
    explicit Channel(socket_t fd,FDEvent events,std::shared_ptr<ChannelContextBase> ctx) : fd_(fd),events_(static_cast<int>(events)),context_(ctx){}
    void writeEventEnable(bool flag);
    [[nodiscard]] bool isWriteEventEnable() const;
    socket_t getSocket() const;
    int getEvents() const;
    std::shared_ptr<ChannelContextBase> getContext();


private:
    socket_t fd_;//文件描述符
    //读写可以都发生，0x02是第二位1，0x04是第三位1，用按位操作来判断而不是枚举
    int events_;//处理时间描述
    std::shared_ptr<ChannelContextBase> context_;

};



#endif //CHANNEL_H
