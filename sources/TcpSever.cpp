//
// Created by 洛琪希 on 25-5-3.
//

#include "../headers/TcpSever.h"

#include "../headers/TcpConnection.h"
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

TcpSever::TcpSever(unsigned short port, int threadNum)
{
    port_ = port;
    threadNum_ = threadNum;
    mainLoop_ = std::make_shared<EventLoop>();
    mainLoop_->initWakeupChannel();
    threadPool_ = std::make_shared<ThreadPool>(mainLoop_,threadNum);
}

int TcpSever::readCallback()
{
    socket_t cfd = accept(lfd_,NULL,NULL);
    std::shared_ptr<EventLoop> evLoop = threadPool_->takeWorkerEventLoop();
    std::shared_ptr<TcpConnection> conn = std::make_shared<TcpConnection>(cfd,evLoop);
    conn->init();
    return 0;
}

void TcpSever::run()
{
    threadPool_->run();
    std::shared_ptr<Channel> channel = std::make_shared<Channel>(lfd_,FDEvent::ReadEvent,shared_from_this());
    mainLoop_->addTask(channel,ElemType::ETADD);
    mainLoop_->run();
}

void TcpSever::setListen()
{
    lfd_ = socket(AF_INET,SOCK_STREAM,0);
    if(lfd_ == -1)
    {
        std::cerr << "setListen-->socket error" << std::endl;
    }
    //设置端口复用
    int opt = 1;
    int ret = setSocketOption(lfd_,1,SO_REUSEADDR,&opt,sizeof(opt));
    if(ret == -1)
    {
        std::cerr << "setListen-->setSocketOpt error" << std::endl;
        closeSocket(lfd_);
        return;
    }
    //绑定端口
    sockaddr_in addr{0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bindSocket(lfd_,reinterpret_cast<sockaddr*>(&addr),sizeof(addr));
    if(ret == -1)
    {
        std::cerr << "setListen-->bindSocket error" << std::endl;
        closeSocket(lfd_);
        return;
    }
    ret = listen(lfd_,128);
    if(ret == -1)
    {
        std::cerr << "setListen-->bindSocket error" << std::endl;
        closeSocket(lfd_);
        return;
    }
}

int TcpSever::setSocketOption(socket_t sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
#ifdef _WIN32
    return setsockopt(sockfd, level, optname, reinterpret_cast<const char*>(optval), static_cast<int>(optlen));
#else
    return setsockopt(sockfd, level, optname, optval, optlen);
#endif
}

void TcpSever::closeSocket(socket_t sockfd)
{
#ifdef _WIN32
    if (sockfd != INVALID_SOCKET) {
        closesocket(sockfd);
    }
#else
    if (sockfd >= 0) {
        close(sockfd);
    }
#endif
}

int TcpSever::bindSocket(socket_t sockfd, const sockaddr *addr, socklen_t addrlen)
{
#ifdef _WIN32
    return bind(sockfd, addr, static_cast<int>(addrlen));
#else
    return bind(sockfd, addr, addrlen);
#endif
}
