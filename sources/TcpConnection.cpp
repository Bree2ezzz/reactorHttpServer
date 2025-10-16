//
// Created by 洛琪希 on 25-5-11.
//

#include "../headers/TcpConnection.h"
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include "../headers/IocpDispatcher.h"
#endif

TcpConnection::TcpConnection(socket_t fd, std::shared_ptr<EventLoop> evLoop)
{
    evLoop_ = evLoop;
    readBuf_ = std::make_shared<Buffer>(10240);
    writeBuf_ = std::make_shared<Buffer>(10240);
    request_ = std::make_shared<HttpRequest>();
    response_ = std::make_shared<HttpResponse>();
    name_ = "Connection-" + std::to_string(fd);
    fd_ = fd;
    
#ifdef _WIN32
    // 初始化IOCP重叠结构
    readOverlapped_ = std::make_shared<IocpOverlapped>();
    memset(&readOverlapped_->overlapped, 0, sizeof(OVERLAPPED));
    readOverlapped_->opType = IocpOpType::Read;
    readOverlapped_->socket = fd;
    readOverlapped_->wsaBuf.buf = readOverlapped_->buffer;
    readOverlapped_->wsaBuf.len = sizeof(readOverlapped_->buffer);
    readOverlapped_->bytesTransferred = 0;
#endif
}

TcpConnection::~TcpConnection()
{
#ifdef _WIN32
    if (fd_ != INVALID_SOCKET) {
        closesocket(fd_);
    }
#else
    if (fd_ >= 0) {
        close(fd_);
    }
#endif
}

void TcpConnection::init()
{
    channel_ = std::make_shared<Channel>(fd_,FDEvent::ReadEvent,shared_from_this());
    evLoop_->addTask(channel_,ElemType::ETADD);
    
#ifdef _WIN32
    // Windows下发起第一次异步读操作
    postRead();
#endif
}

int TcpConnection::writeCallback()
{
#ifdef _WIN32
#else
    socket_t socket = channel_->getSocket();
    while (writeBuf_->readableSize() > 0) {
        int sent = writeBuf_->sendData(socket);
        if (sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 缓冲区满，等待下次可写事件
                break;
            } else {
                //错误，关闭连接
                evLoop_->addTask(channel_, ElemType::ETDELETE);
                return 0;
            }
        }
    }

    // 发送完成后关闭连接
    if (writeBuf_->readableSize() == 0) {
        evLoop_->addTask(channel_, ElemType::ETDELETE);
    }
#endif

    return 0;
}

int TcpConnection::readCallback()
{
#ifdef _WIN32
    // Windows下，这个方法由IOCP完成通知调用
    // 实际的字节数已经在onReadComplete中处理
    onReadComplete(readOverlapped_->bytesTransferred);
#else
    // Linux下，使用传统的同步读取
    socket_t socket = channel_->getSocket();
    //边缘触发。循环读取直到无数据
    while(true)
    {
        int count = readBuf_->socketRead(socket);
        if (count > 0) {
            processHttpRequest();
        } else if (count == 0) {
            // 连接关闭
            evLoop_->addTask(channel_, ElemType::ETDELETE);
            break;
        } else {
            // EAGAIN，没有更多数据可读
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                // 真正的错误，关闭连接
                evLoop_->addTask(channel_, ElemType::ETDELETE);
                break;
            }
        }
    }

#endif
    return 0;
}

#ifdef _WIN32
void TcpConnection::postRead()
{
    // 重置重叠结构
    memset(&readOverlapped_->overlapped, 0, sizeof(OVERLAPPED));
    readOverlapped_->bytesTransferred = 0;
    
    DWORD flags = 0;
    DWORD bytesReceived = 0;
    
    // 发起异步读操作
    int result = WSARecv(
        fd_,
        &readOverlapped_->wsaBuf,
        1,
        &bytesReceived,
        &flags,
        &readOverlapped_->overlapped,
        NULL
    );
    
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            // 真正的错误，关闭连接
            SPDLOG_ERROR("WSARecv failed: {}", error);
            evLoop_->addTask(channel_, ElemType::ETDELETE);
        }
    }
}

void TcpConnection::onReadComplete(DWORD bytesTransferred)
{
    socket_t socket = channel_->getSocket();
    if (bytesTransferred == 0) {
        // 连接关闭
        evLoop_->addTask(channel_, ElemType::ETDELETE);
        return;
    }
    
    // 将IOCP缓冲区的数据复制到readBuf_
    readBuf_->appendData(readOverlapped_->buffer, bytesTransferred);
    
    // 处理HTTP请求
    request_->reset();
    response_->reset();
    
    bool flag = request_->parseRequest(readBuf_, response_, writeBuf_, socket);
    if (!flag) {
        std::string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
        // 统一使用同步发送
        send(socket, errMsg.c_str(), errMsg.size(), 0);
    }
    
    // 继续发起下一次异步读操作
    postRead();
}
#endif
void TcpConnection::processHttpRequest()
{
    request_->reset();
    response_->reset();
    bool flag = request_->parseRequest(readBuf_,response_,writeBuf_,fd_);
    if (!flag) {
        std::string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
        writeBuf_->appendString(errMsg.c_str());
    }
    //有数据写，启用写事件监听
    if (writeBuf_->readableSize() > 0) {
        channel_->writeEventEnable(true);
        evLoop_->addTask(channel_, ElemType::ETMODIFY);
    }
}