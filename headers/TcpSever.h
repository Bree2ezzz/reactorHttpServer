//
// Created by 洛琪希 on 25-5-3.
//

#ifndef TCPSEVER_H
#define TCPSEVER_H
#include <memory>

#include "EventLoop.h"
#include "ThreadPool.h"
#include "Channel.h"
#include <iostream>
class TcpSever : public ChannelContextBase , public std::enable_shared_from_this<TcpSever>
{

public:
    TcpSever(unsigned short port,int threadNum);
    ~TcpSever() = default;
    int readCallback() override;//连接回调函数
    void run();//启动服务器
    void setListen();
    int setSocketOption(socket_t sockfd, int level, int optname, const void* optval, socklen_t optlen);
    void closeSocket(socket_t sockfd);
    int bindSocket(socket_t sockfd, const struct sockaddr* addr, socklen_t addrlen);

private:

    int threadNum_;
    std::shared_ptr<EventLoop> mainLoop_;
    std::shared_ptr<ThreadPool> threadPool_;
    unsigned int port_;
    socket_t lfd_;//用于监听的文件描述符

};



#endif //TCPSEVER_H
