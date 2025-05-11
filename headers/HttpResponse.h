//
// Created by 洛琪希 on 25-5-7.
//

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "Buffer.h"
#include <map>
#include <functional>
#include <string>
enum class StatusCode
{
    Unknown,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};

class HttpResponse {
public:
    std::function<void(const char*,std::shared_ptr<Buffer>,socket_t)> sendDataFunc;
    HttpResponse();
    ~HttpResponse() = default;
    void addHeader(const std::string key,const std::string value);
    void prepareMsg(std::shared_ptr<Buffer> sendBuf, socket_t socket);
    inline void setFileName(const char* fileName)
    {
        fileName_ = fileName;
    }
    inline void setStatuCode(StatusCode code)
    {
        statusCode_ = code;

    }

private:
    //状态码
    StatusCode statusCode_;
    //响应头 - 键值对
    std::map<std::string,std::string> headers_;
    //状态码与对应状态描述
    const std::map<int, std::string> statuMap_ = {
        {200, "OK"},
        {301, "Moved Permanently"},
        {302, "Moved Temporarily"},
        {400, "Bad Request"},
        {404, "Not Found"}
    };
    std::string fileName_{};

};



#endif //HTTPRESPONSE_H
