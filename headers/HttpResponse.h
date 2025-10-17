//
// Created by 洛琪希 on 25-5-7.
//

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "Buffer.h"
#include <map>
#include <functional>
#include <string>
#include <cstddef>
enum class StatusCode
{
    Unknown,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};

#ifdef _WIN32
using SendDataFunc = std::function<void(const char*,std::shared_ptr<Buffer>,socket_t)>;
#endif

enum class ResponseBodyType
{
    None,
    Memory,
    File
};

class HttpResponse {
public:
#ifdef _WIN32
    SendDataFunc sendDataFunc;
#endif
    HttpResponse();
    ~HttpResponse() = default;
    void addHeader(const std::string key,const std::string value);
    void prepareMsg(std::shared_ptr<Buffer> sendBuf, socket_t socket);
    void reset();
    inline void setFileName(const char* fileName)
    {
        fileName_ = fileName;
    }
    inline void setStatuCode(StatusCode code)
    {
        statusCode_ = code;

    }
    void setBodyContent(const std::string& body);
    void setFileBody(const std::string& filePath, std::size_t fileSize);
    void setShouldClose(bool flag) { shouldClose_ = flag; }
    bool shouldClose() const { return shouldClose_; }
    ResponseBodyType bodyType() const { return bodyType_; }
    const std::string& bodyContent() const { return bodyContent_; }
    std::size_t fileSize() const { return fileSize_; }
    const std::string& getFileName() const { return fileName_; }

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
    std::string bodyContent_{};
    std::size_t fileSize_{0};
    ResponseBodyType bodyType_{ResponseBodyType::None};
    bool shouldClose_{true};

};



#endif //HTTPRESPONSE_H
