//
// Created by 洛琪希 on 25-5-6.
//

#include "../headers/Buffer.h"

#include <cstdlib>
#include <string.h>

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
    if(writeableSize() >= size)
    {
        return;
    }
    else if(readPos_ + writeableSize() >= size)
    {
        int readable = readableSize();
        memcpy(data_ + readPos_,data_,readable);
        readPos_ = 0;
        writePos_ = readable;
    }
    else
    {
        void* temp = realloc(data_,capacity_ + size);
        if(temp == NULL)
        {
            return;
        }
        data_ = (char*)temp;
        capacity_ += size;
    }
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
    if (writeable == 0)
    {
        return 0;
    }

    int result = recv(fd, data_ + writePos_, writeable, 0);

    if (result > 0)
    {
        writePos_ += result;
    }

    return result; // >0读到了，==0对端关闭，<0错误
}

char * Buffer::data()
{
    return data_;
}

void Buffer::removeOneLine()
{
    char* crlf = findCRLF();
    if (crlf != nullptr)
    {
        readPos_ = static_cast<int>(crlf - data()) + 2; // 移动readPos_，跳过这一行（包括\r\n）
    }
}



