//
// Created by 洛琪希 on 25-5-11.
//

#include "../headers/TcpConnection.h"

TcpConnection::TcpConnection(socket_t fd, std::shared_ptr<EventLoop> evLoop)
{
    evLoop_ = evLoop;
    readBuf_ = std::make_shared<Buffer>(10240);
    writeBuf_ = std::make_shared<Buffer>(10240);
    request_ = std::shared_ptr<HttpRequest>();
    response_ = std::shared_ptr<HttpResponse>();
    name_ = "Connection-" + std::to_string(fd);
    fd_ = fd;
}

void TcpConnection::init()
{
    channel_ = std::make_shared<Channel>(fd_,FDEvent::ReadEvent,shared_from_this());
    evLoop_->addTask(channel_,ElemType::ETADD);
}

int TcpConnection::writeCallback()
{
    int count = writeBuf_->sendData(channel_->getSocket());
    if(count > 0)
    {
        if(writeBuf_->readableSize() == 0)
        {
            channel_->writeEventEnable(false);
            evLoop_->addTask(channel_,ElemType::ETDELETE);
        }
    }
    return 0;
}

int TcpConnection::readCallback()
{
    socket_t socket = channel_->getSocket();
    int count = readBuf_->socketRead(socket);
    if(count > 0)
    {
        bool flag = request_->parseRequest(readBuf_,response_,writeBuf_,socket);
        if(!flag)
        {
            std::string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
            writeBuf_->appendString(errMsg.c_str());
        }
    }
    else
    {
        evLoop_->addTask(channel_,ElemType::ETDELETE);
    }
    return 0;
}
