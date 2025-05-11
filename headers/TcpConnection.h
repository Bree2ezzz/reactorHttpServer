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
class TcpConnection : public ChannelContextBase, public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(socket_t fd,std::shared_ptr<EventLoop> evLoop);
    ~TcpConnection() = default;
    void init();
    int writeCallback() override;
    int readCallback() override;
private:
    std::string name_;
    std::shared_ptr<EventLoop> evLoop_;
    std::shared_ptr<Channel> channel_;
    std::shared_ptr<Buffer> readBuf_;
    std::shared_ptr<Buffer> writeBuf_;
    std::shared_ptr<HttpRequest> request_;
    std::shared_ptr<HttpResponse> response_;
    socket_t fd_;
};



#endif //TCPCONNECTION_H
