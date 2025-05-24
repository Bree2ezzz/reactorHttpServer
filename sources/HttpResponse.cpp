//
// Created by 洛琪希 on 25-5-7.
//

#include "../headers/HttpResponse.h"

HttpResponse::HttpResponse()
{
    statusCode_ = StatusCode::Unknown;
}

void HttpResponse::addHeader(const std::string key, const std::string value)
{
    if(key.empty() || value.empty())
    {
        return;
    }
    headers_.emplace(key,value);
}

void HttpResponse::prepareMsg(std::shared_ptr<Buffer> sendBuf, socket_t socket)
{
    //组织数据，写入Buf中，按照http响应格式
    char tmp[1024];
    int code  = static_cast<int>(statusCode_);
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", code, statuMap_.at(code).data());
    sendBuf->appendString(tmp);
    for(auto & it : headers_)
    {
        sprintf(tmp, "%s: %s\r\n", it.first.data(),it.second.data());
        sendBuf->appendString(tmp);
    }
    sendBuf->appendString("\r\n");
    sendBuf->sendData(socket);
    sendDataFunc(fileName_.c_str(),sendBuf,socket);
}

void HttpResponse::reset()
{
    statusCode_ = StatusCode::Unknown;
    headers_.clear();
    fileName_.clear();
    sendDataFunc = nullptr;
}

