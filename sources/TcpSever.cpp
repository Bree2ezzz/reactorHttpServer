//
// Created by 洛琪希 on 25-5-3.
//

#include "../headers/TcpSever.h"
#include <spdlog/spdlog.h>
#include "../headers/TcpConnection.h"
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#endif

TcpSever::TcpSever(unsigned short port, int threadNum)
{
#ifdef _WIN32
    // 初始化Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        SPDLOG_ERROR("WSAStartup failed: {}", result);
        exit(1);
    }
#endif

    port_ = port;
    threadNum_ = threadNum;
    auto loop = std::make_shared<EventLoop>();
    mainLoop_ = loop;//主事件循环
    try{
        mainLoop_->init();
        mainLoop_->initWakeupChannel();
    }
    catch(const std::exception& e)
    {
        SPDLOG_ERROR("init mainLoop failed:{}",e.what());
    }
    threadPool_ = std::make_shared<ThreadPool>(mainLoop_,threadNum);
    SPDLOG_INFO("TcpSever init success");
}

int TcpSever::readCallback()
{
    //处理新连接的接受
    socket_t cfd = accept(lfd_,NULL,NULL);
#ifdef _WIN32
    if(cfd == INVALID_SOCKET)
    {
        SPDLOG_ERROR("accept failed: {}", WSAGetLastError());
        return -1;
    }
#else
    if(cfd == -1)
    {
        SPDLOG_ERROR("accept failed");
        return -1;
    }
    int flags = fcntl(cfd, F_GETFL, 0);
    if (flags == -1)
    {
        SPDLOG_WARN("fcntl F_GETFL failed for fd {}: {}", cfd, strerror(errno));
    }
    else if (fcntl(cfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        SPDLOG_WARN("fcntl F_SETFL failed for fd {}: {}", cfd, strerror(errno));
    }
#endif
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
#ifdef _WIN32
    if(lfd_ == INVALID_SOCKET)
#else
    if(lfd_ == -1)
#endif
    {
        std::cerr << "setListen-->socket error" << std::endl;
        return;
    }
    //设置端口复用
    int opt = 1;
    int ret = setSocketOption(lfd_,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    if(ret == -1)
    {
        std::cerr << "setListen-->setSocketOpt error" << std::endl;
#ifdef _WIN32
        int error_code = WSAGetLastError(); // 获取具体的错误码
        std::cerr << "setsockopt failed for sockfd " << error_code;
#endif
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
    // 检查是否是设置端口复用选项
    if (level == SOL_SOCKET && optname == SO_REUSEADDR)
    {
        int ret;
#ifdef _WIN32
        //据说SO_EXCLUSIVEADDRUSE更安全
        ret = setsockopt(sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(optval), static_cast<int>(optlen));
        if (ret == SOCKET_ERROR) {
            std::cerr << "Error setting SO_EXCLUSIVEADDRUSE on Windows: " << WSAGetLastError() << std::endl;
        }
        return ret;
#else
        // 在 Linux/Unix 上，SO_REUSEADDR 行为符合预期，主要用于处理 TIME_WAIT 状态。
        ret = setsockopt(sockfd, level, optname, optval, optlen);
        if (ret == -1) {
            perror("Error setting SO_REUSEADDR on Linux");
        }
        return ret;
#endif
    }
    else
    {
        // 对于其他套接字选项，直接调用底层的 setsockopt
#ifdef _WIN32
        return setsockopt(sockfd, level, optname, reinterpret_cast<const char*>(optval), static_cast<int>(optlen));
#else
        return setsockopt(sockfd, level, optname, optval, optlen);
#endif
    }
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

TcpSever::~TcpSever()
{
    closeSocket(lfd_);
    
#ifdef _WIN32
    // 清理Winsock
    WSACleanup();
#endif
}
