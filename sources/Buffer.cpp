//
// Created by 洛琪希 on 25-5-6.
//

#include "../headers/Buffer.h"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <string.h>
#include <algorithm>

Buffer::Buffer(int size) : capacity_(size),readPos_(0),writePos_(0)
{
    data_ = (char*)malloc(size);
}

Buffer::~Buffer()
{
    free(data_);
}

int Buffer::sendData(socket_t socket)
{
    int readAble = readableSize();
    if(readAble > 0)
    {
#ifdef _WIN32
        int count = send(socket, data_ + readPos_, readableSize(), 0);
#else
        // 发送数据，使用MSG_NOSIGNAL避免SIGPIPE信号
        int count = send(socket, data_ + readPos_, readableSize(), MSG_NOSIGNAL);
#endif
        if(count > 0)
        {
            readPos_ += count;
            // 如果还有数据未发送完，记录日志
            if (readableSize() > 0) {
                SPDLOG_INFO("data not fully sent,remaining bytes: {}", readableSize());
            }
        }
        else if (count == 0)
        {
            // 连接可能已关闭
            SPDLOG_INFO("send 0 bytes,connection may be closed");
        }
        else
        {
            // 发送错误
            SPDLOG_ERROR("send data error: {}", strerror(errno));
        }
        return count;
    }
    return 0;
}


char * Buffer::findCRLF()
{
    int readable = readableSize();
    for (int i = readPos_; i + 1 < writePos_; ++i) {
        if (data_[i] == '\r' && data_[i + 1] == '\n') {
            return data_ + i;
        }
    }
    return nullptr;
}

void Buffer::externRoom(int size)
{
    int readable = readableSize();        // 当前未读数据量
    int writable = writeableSize();       // 当前可写空间
    int front_space = readPos_;           // 前面被读掉的空间
    
    // 如果当前可写空间足够，直接返回
    if (writable >= size) {
        return;
    }
    
    // 通过数据搬移复用前面的空间
    if (front_space + writable >= size) {
        memmove(data_, data_ + readPos_, readable);
        readPos_ = 0;
        writePos_ = readable;
        return;
    }

    // 需要扩容：简单直观的策略 - 翻倍扩容直到满足需求
    int newCap = capacity_;
    int required_total_space = readable + size;
    
    // 确保新容量足够大
    while (newCap < required_total_space) {
        newCap = (newCap == 0) ? 1024 : newCap * 2;  // 从1KB开始，每次翻倍
    }

    // 分配新内存并复制数据
    char* newData = (char*)malloc(newCap);
    if (!newData) {
        SPDLOG_ERROR("Failed to allocate memory in Buffer::externRoom with malloc");
        return;
    }
    
    // 只复制有效数据
    if (readable > 0) {
        memcpy(newData, data_ + readPos_, readable);
    }
    
    free(data_);
    data_ = newData;
    capacity_ = newCap;
    readPos_ = 0;
    writePos_ = readable;
}

int Buffer::appendString(const char *data, int size)
{
    if(data == nullptr || size < 0)
    {
        return -1;
    }
    externRoom(size);
    memcpy(data_ + writePos_,data,size);
    writePos_ += size;
    return 0;
}

int Buffer::appendString(const char *data)
{
    return appendString(data,strlen(data));
}


int Buffer::socketRead(socket_t fd)
{
    int writeable = writeableSize();
    int result = recv(fd, data_ + writePos_, writeable, 0);

    if (result > 0)
    {
        writePos_ += result;
    }

    return result;
}

char * Buffer::data()
{
    return data_;
}

void Buffer::removeOneLine()
{
    //这个函数是跳过一个\r\n，不是真正的释放内存
    char* crlf = findCRLF();
    if (crlf) {
        readPos_ = (crlf - data_) + 2;  // 跳过一行（含 \r\n）
    } else {
        // 若未找到换行，不做推进（防止越界）
        SPDLOG_WARN("removeOneLine(): does not find \\r\\n,Skip one line error");
    }
}

void Buffer::reset()
{
    readPos_ = 0;
    writePos_ = 0;
}

// IOCP相关方法实现
int Buffer::readData(socket_t fd)
{
    return socketRead(fd);
}

void Buffer::retrieve(int len)
{
    if (len <= 0) return;
    if (len >= readableSize()) {
        reset();
    } else {
        readPos_ += len;
    }
}

void Buffer::appendData(const char* data, int len)
{
    appendString(data, len);
}


