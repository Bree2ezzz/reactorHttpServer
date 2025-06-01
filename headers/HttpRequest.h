//
// Created by 洛琪希 on 25-5-7.
//

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
#include <string>
#include <map>

#include "Buffer.h"
#include "HttpResponse.h"

//记录当前解析状态
enum class ProcessState:char
{
    ParseReline,
    ParseReHeaders,
    ParseReBody,
    ParseReDone
};
class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest() = default;
    void reset();
    ProcessState getState();
    void addHeader(std::string key,std::string value);
    std::string getHeader(std::string key);
    //解析函数
    bool parseRequsetLine(std::shared_ptr<Buffer> buffer);
    bool parseRequestHeader(std::shared_ptr<Buffer> buffer);
    bool parseRequest(std::shared_ptr<Buffer> readBuf,std::shared_ptr<HttpResponse> response,std::shared_ptr<Buffer> sendBuf,socket_t socket);
    bool processRequest(std::shared_ptr<HttpResponse> response);

    //解码字符串
    static std::string decodeMsg(const std::string& from);
    static std::string getFileType(const std::string& name);

    //发送数据
    static void sendDir(std::string dirName, std::shared_ptr<Buffer> sendBuf, socket_t cfd);
    static void sendFile(std::string fileName, std::shared_ptr<Buffer> sendBuf, socket_t cfd);
    
    //生成目录HTML内容
    static std::string generateDirHTML(const std::string& dirName);
private:
//请求行数据
    std::string method_;
    std::string url_;
    std::string version_;

//请求头--》键值对
    std::map<std::string,std::string> headers_;

    ProcessState curState_;
};



#endif //HTTPREQUEST_H
