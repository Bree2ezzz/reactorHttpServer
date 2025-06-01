//
// Created by 洛琪希 on 25-5-11.
//

#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "Channel.h"
#include "Buffer.h"
#include "EventLoop.h"
#include <string>
#include "HttpRequest.h"
#include "HttpResponse.h"

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
// 前向声明
struct IocpOverlapped;
#endif

class TcpConnection : public ChannelContextBase, public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(socket_t fd,std::shared_ptr<EventLoop> evLoop);
    ~TcpConnection();
    void init();
    int writeCallback() override;
    int readCallback() override;
    
#ifdef _WIN32
    // IOCP相关方法
    void postRead();  // 发起异步读操作
    void onReadComplete(DWORD bytesTransferred);  // 读完成回调
#endif

private:
    std::string name_;
    std::shared_ptr<EventLoop> evLoop_;
    std::shared_ptr<Channel> channel_;
    std::shared_ptr<Buffer> readBuf_;
    std::shared_ptr<Buffer> writeBuf_;
    std::shared_ptr<HttpRequest> request_;
    std::shared_ptr<HttpResponse> response_;
    socket_t fd_;
    
#ifdef _WIN32
    // IOCP相关成员
    std::shared_ptr<IocpOverlapped> readOverlapped_;
#endif
};

#endif //TCPCONNECTION_H
