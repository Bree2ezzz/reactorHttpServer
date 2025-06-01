#ifndef IOCPDISPATCHER_H
#define IOCPDISPATCHER_H

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

#include "Dispatcher.h"
#include "Channel.h"
#include "EventLoop.h"

// IOCP操作类型
enum class IocpOpType {
    Accept,
    Connect,
    Read,
    Write
};

// IOCP重叠结构体
struct IocpOverlapped {
    OVERLAPPED overlapped;
    IocpOpType opType;
    WSABUF wsaBuf;//结构体，内容是缓冲区大小以及缓冲区的地址，初始化时要对内部指针指向下面的buffer
    socket_t socket;
    char buffer[4096];  // 缓冲区
    DWORD bytesTransferred;  // 实际传输的字节数
};

class IocpDispatcher : public Dispatcher
{
public:
    IocpDispatcher(std::shared_ptr<EventLoop> evLoop);
    ~IocpDispatcher();
    
    int add() override;
    int remove() override;
    int modify() override;
    int dispatch(int timeout) override;

private:
    HANDLE iocpHandle_;  // IOCP句柄
    const int maxNode_ = 1024;
    
    // 存储与每个socket关联的重叠结构
    std::map<socket_t, std::shared_ptr<IocpOverlapped>> overlappedMap_;
};

#endif // _WIN32
#endif // IOCPDISPATCHER_H