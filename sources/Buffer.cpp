//
// Created by 洛琪希 on 25-5-6.
//

#include "../headers/Buffer.h"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <string.h>
#include <algorithm> // For std::max

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
        int count = send(socket, data_ + readPos_, readableSize(), MSG_NOSIGNAL);
#endif
        if(count > 0)
        {
            readPos_ += count;
            usleep(1);
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
    if (writable >= (int)size) {
        // 当前空间够写，不需要处理
        return;
    }
    if (writable >= (int)size) {
        // 当前空间够写，不需要处理
        return;
    }

    if (front_space + writable >= (int)size) {
        // 通过数据搬移复用前面的空间
        memmove(data_, data_ + readPos_, readable);
        readPos_ = 0;
        writePos_ = readable;
        return;
    }

    // 都不够：扩容
    int newCap = capacity_;
    // 当前可读数据量 + 需要的额外空间 size，才是扩容后至少需要的总空间
    // 确保 newCap 至少是当前容量加上额外需要的 size，或者至少是当前可读数据 + size
    // 一个更安全的扩容逻辑：
    int required_total_space = readable + size; // 需要的总空间
    // 如果当前容量减去已读数据后，剩余空间不足以放下新数据，则需要扩容
    // 或者更直接地，如果总容量不足以容纳 (已有的可读数据 + 新来的数据)
    if (capacity_ < required_total_space) { // 如果当前总容量 < (已有的数据 + 新数据)
        while (newCap < required_total_space) { // 确保新容量足够大
            newCap = (newCap == 0) ? std::max(128, required_total_space) : (newCap * 2);
        }
    } else if (capacity_ - writePos_ + readPos_ < size) { // 即使总容量够，但如果把数据移到前面后，尾部剩余空间还是不够size，也需要扩容
        // (capacity_ - writePos_) 是尾部空闲，readPos_ 是头部空闲
        // (capacity_ - writePos_ + readPos_) 是总空闲空间
        // 如果总空闲空间还是不够，也需要扩容到 required_total_space
         while (newCap < required_total_space) {
            newCap = (newCap == 0) ? std::max(128, required_total_space) : (newCap * 2);
        }
    }


    char* newData = (char*)malloc(newCap);
    if (!newData) {
        SPDLOG_ERROR("Failed to allocate memory in Buffer::externRoom with malloc");
        // 最好抛出异常或采取更激烈的错误处理
        return; // 简单的返回可能隐藏问题
    }
    if (readable > 0) {
        memcpy(newData, data_ + readPos_, readable);
    }
    free(data_); // 释放旧内存
    data_ = newData;
    capacity_ = newCap;
    readPos_ = 0;       // 重置 readPos_
    writePos_ = readable; // writePos_ 是已拷贝的数据量
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
        // 可加日志帮助调试：
        SPDLOG_WARN("removeOneLine(): 未找到 \\r\\n，跳过失败");
    }
}



