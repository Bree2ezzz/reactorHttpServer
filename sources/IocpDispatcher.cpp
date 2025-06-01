#include "../headers/IocpDispatcher.h"
#include <spdlog/spdlog.h>

#ifdef _WIN32

IocpDispatcher::IocpDispatcher(std::shared_ptr<EventLoop> evLoop) : Dispatcher(evLoop)
{
    // 创建IOCP句柄
    iocpHandle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (iocpHandle_ == NULL)
    {
        SPDLOG_ERROR("CreateIoCompletionPort failed: {}", GetLastError());
        exit(0);
    }
}

IocpDispatcher::~IocpDispatcher()
{
    if (iocpHandle_ != NULL)
    {
        CloseHandle(iocpHandle_);
    }
}

int IocpDispatcher::add()
{
    // 将socket与IOCP关联
    if (channel_ == nullptr)
    {
        SPDLOG_ERROR("Channel is null");
        return -1;
    }

    socket_t sockfd = channel_->getSocket();
    
    // 检查是否是监听socket（通过getsockopt检查SO_ACCEPTCONN）
    int isListening = 0;
    int optlen = sizeof(isListening);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ACCEPTCONN, (char*)&isListening, &optlen) == 0 && isListening)
    {
        // 这是监听socket，不关联到IOCP
        SPDLOG_INFO("Listening socket detected, will use select for monitoring");
        return 0;
    }
    
    // 对于非监听socket，关联到IOCP
    HANDLE ret = CreateIoCompletionPort((HANDLE)sockfd, iocpHandle_, (ULONG_PTR)sockfd, 0);
    if (ret == NULL)
    {
        DWORD error = GetLastError();
        SPDLOG_ERROR("CreateIoCompletionPort failed for socket {}: {}", sockfd, error);
        return -1;
    }
    
    return 0;
}

int IocpDispatcher::remove()
{
    if (channel_ == nullptr)
    {
        return -1;
    }
    
    socket_t sockfd = channel_->getSocket();
    auto it = overlappedMap_.find(sockfd);
    if (it != overlappedMap_.end())
    {
        overlappedMap_.erase(it);
    }
    
    return 0;
}

int IocpDispatcher::modify()
{
    return 0;
}

int IocpDispatcher::dispatch(int timeout)
{
    // 获取 shared_ptr
    auto evLoopShared = evLoop_.lock();
    if (!evLoopShared) {
        // EventLoop 已被销毁，返回
        return -1;
    }
    
    // 首先检查监听socket是否有连接到达
    fd_set readfds;
    FD_ZERO(&readfds);
    socket_t maxfd = 0;
    bool hasListeningSockets = false;
    
    // 遍历所有channel，找出监听socket
    for (const auto& pair : evLoopShared->getChannelMap())
    {
        socket_t sockfd = pair.first;
        int isListening = 0;
        int optlen = sizeof(isListening);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ACCEPTCONN, (char*)&isListening, &optlen) == 0 && isListening)
        {
            FD_SET(sockfd, &readfds);
            if (sockfd > maxfd) maxfd = sockfd;
            hasListeningSockets = true;
            // SPDLOG_INFO("Found listening socket: {}", sockfd);
        }
    }
    
    // 如果有监听socket，使用select检查
    if (hasListeningSockets)
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000; // 500ms，增加超时时间
        
        int selectResult = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        // SPDLOG_INFO("Select result: {}", selectResult);
        
        if (selectResult > 0)
        {
            // 有监听socket就绪
            for (const auto& pair : evLoopShared->getChannelMap())
            {
                socket_t sockfd = pair.first;
                if (FD_ISSET(sockfd, &readfds))
                {
                    SPDLOG_INFO("Listening socket {} is ready for accept", sockfd);
                    evLoopShared->eventActive(sockfd, static_cast<int>(FDEvent::ReadEvent));
                    return 1; // 处理了一个事件
                }
            }
        }
        else if (selectResult < 0)
        {
            SPDLOG_ERROR("Select failed: {}", WSAGetLastError());
        }
    }
    
    // 然后处理IOCP事件
    DWORD bytesTransferred = 0;
    ULONG_PTR completionKey = 0;
    LPOVERLAPPED overlapped = nullptr;
    
    // 获取完成通知
    BOOL result = GetQueuedCompletionStatus(
        iocpHandle_,
        &bytesTransferred,
        &completionKey,
        &overlapped,
        timeout * 1000
    );
    
    if (overlapped == nullptr)
    {
        // 超时或出错
        if (!result)
        {
            DWORD error = GetLastError();
            if (error != WAIT_TIMEOUT)
            {
                SPDLOG_ERROR("GetQueuedCompletionStatus failed: {}", error);
            }
        }
        return 0;
    }
    
    // 获取自定义的重叠结构
    IocpOverlapped* ioData = CONTAINING_RECORD(overlapped, IocpOverlapped, overlapped);
    socket_t sockfd = (socket_t)completionKey;
    
    // 保存实际传输的字节数
    ioData->bytesTransferred = bytesTransferred;
    
    if (!result || bytesTransferred == 0)
    {
        // 连接关闭或出错
        evLoopShared->eventActive(sockfd, static_cast<int>(FDEvent::ReadEvent) | static_cast<int>(FDEvent::WriteEvent));
        return 0;
    }
    
    // 根据操作类型触发相应事件
    switch (ioData->opType)
    {
    case IocpOpType::Read:
        evLoopShared->eventActive(sockfd, static_cast<int>(FDEvent::ReadEvent));
        break;
    case IocpOpType::Write:
        evLoopShared->eventActive(sockfd, static_cast<int>(FDEvent::WriteEvent));
        break;
    case IocpOpType::Accept:
        // 处理接受连接完成
        evLoopShared->eventActive(sockfd, static_cast<int>(FDEvent::ReadEvent));
        break;
    case IocpOpType::Connect:
        // 处理连接完成
        evLoopShared->eventActive(sockfd, static_cast<int>(FDEvent::WriteEvent));
        break;
    }
    
    return 0;
}

#endif // _WIN32
