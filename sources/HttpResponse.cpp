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
#ifdef _WIN32
    // Windows路径保持原有同步发送行为
    sendBuf->sendData(socket);
    if (sendDataFunc) {
        sendDataFunc(fileName_.c_str(), sendBuf, socket);
    }
#else
    if (bodyType_ == ResponseBodyType::Memory) {
        sendBuf->appendString(bodyContent_.data(), static_cast<int>(bodyContent_.size()));
    }
    (void)socket;
#endif
}

void HttpResponse::reset()
{
    statusCode_ = StatusCode::Unknown;
    headers_.clear();
    fileName_.clear();
#ifdef _WIN32
    sendDataFunc = nullptr;
#endif
    bodyContent_.clear();
    fileSize_ = 0;
    bodyType_ = ResponseBodyType::None;
    shouldClose_ = true;
}

void HttpResponse::setBodyContent(const std::string& body)
{
    bodyContent_ = body;
    bodyType_ = ResponseBodyType::Memory;
    fileSize_ = body.size();
}

void HttpResponse::setFileBody(const std::string& filePath, std::size_t fileSize)
{
    fileName_ = filePath;
    fileSize_ = fileSize;
    bodyType_ = ResponseBodyType::File;
    bodyContent_.clear();
}
