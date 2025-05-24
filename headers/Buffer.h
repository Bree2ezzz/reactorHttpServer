//
// Created by 洛琪希 on 25-5-6.
//

#ifndef BUFFER_H
#define BUFFER_H
#include "Channel.h"


class Buffer {

public:
    Buffer(int size);
    ~Buffer();

    int sendData(socket_t socket);
    //得到可写容量
    inline int writeableSize() const
    {
        return capacity_ - writePos_;
    }
    //得到可读容量
    inline int readableSize() const
    {
        return writePos_ - readPos_;
    }
    char* findCRLF();//返回\r的地址
    void externRoom(int size);
    int appendString(const char* data,int size);
    int appendString(const char* data);
    int socketRead(socket_t fd);
    char* data();
    void removeOneLine();
    void reset();
    inline int readPos()
    {
        return readPos_;
    }
private:
    char* data_;
    int capacity_;
    int readPos_;
    int writePos_;
};





#endif //BUFFER_H
