//
// Created by 洛琪希 on 25-5-7.
//

#include "../headers/HttpRequest.h"
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>     // for std::put_time
#include <fstream>
#include <system_error>
#include <spdlog/spdlog.h>
#include <algorithm> // For std::min (already present, ensure it is, or add if not)
#include <cstddef>   // For ptrdiff_t

HttpRequest::HttpRequest()
{
    reset();
}


void HttpRequest::reset()
{
    method_ = url_ = version_ = "";
    curState_ = ProcessState::ParseReline;
    headers_.clear();
}

ProcessState HttpRequest::getState()
{
    return curState_;
}

void HttpRequest::addHeader(std::string key, std::string value)
{
    if(key.empty() || value.empty())
    {
        return;
    }
    headers_.emplace(key,value);
}

std::string HttpRequest::getHeader(const std::string key)
{
    if(!key.empty())
    {
        return headers_[key];
    }
    return "";
}

bool HttpRequest::parseRequsetLine(std::shared_ptr<Buffer> buffer)
{
    char* end = buffer->findCRLF();
    if (end == nullptr) 
    {
        // SPDLOG_DEBUG("parseRequsetLine: No CRLF found in buffer.");
        return false;
    }
    char* start = buffer->data() + buffer->readPos();
    // SPDLOG_DEBUG("parseRequsetLine: start={}, end={}", std::string(start, std::min(static_cast<ptrdiff_t>(20), end-start)), std::string(end,2));

    //提取Method
    char* space = std::find(start, end, ' ');
    if (space == end) 
    {
        SPDLOG_DEBUG("parseRequsetLine: No space found for METHOD.");
        return false;
    }
    method_.assign(start, space);
    SPDLOG_DEBUG("parseRequsetLine: Method: {}", method_);

    //提取Url
    char* pathStart = space + 1;
    if (pathStart >= end) // 检查 pathStart 是否越界
    {
        // SPDLOG_DEBUG("parseRequsetLine: pathStart is out of bounds after first space.");
        return false;
    }
    space = std::find(pathStart, end, ' ');
    if (space == end) 
    {
        // SPDLOG_DEBUG("parseRequsetLine: No space found for URL.");
        return false;
    }
    url_.assign(pathStart, space);

    //提取Version
    char* versionStart = space + 1;
    if (versionStart >= end) // 检查 versionStart 是否越界
    {
        SPDLOG_DEBUG("parseRequsetLine: versionStart is out of bounds after second space.");
        return false;
    }
    version_.assign(versionStart, end);

    buffer->removeOneLine();
    curState_ = ProcessState::ParseReHeaders;
    SPDLOG_DEBUG("Exit parseRequsetLine: Success.");
    return true;

}

bool HttpRequest::parseRequestHeader(std::shared_ptr<Buffer> buffer)
{
    char* crlf = buffer->findCRLF();
    if (crlf == nullptr)
    {
        // SPDLOG_DEBUG("parseRequestHeader: No CRLF found in buffer.");
        return false;
    }
    char* lineStart = buffer->data() + buffer->readPos();

    if (lineStart == crlf)
    {
        // 空行（\r\n），说明请求头结束
        buffer->removeOneLine(); // 推进readPos_
        // 通知状态机转到下一阶段（比如解析请求体、生成响应）
        curState_ = ProcessState::ParseReDone;
        SPDLOG_DEBUG("parseRequestHeader: Empty line found. Headers done.");
        return true;
    }

    char* colon = std::find(lineStart, crlf, ':');
    if (colon == crlf || lineStart >= colon) // 检查是否有冒号，并且key不为空
    {
        SPDLOG_DEBUG("parseRequestHeader: No colon found, or key is empty.");
        return false;
    }

    std::string key(lineStart, colon);
    char* valueStart = colon + 1;
    // valueStart 不会大于 crlf，因为 colon < crlf。如果 colon + 1 == crlf，value 是空字符串，合法。
    std::string value(valueStart, crlf);
    
    // 去掉value前导空格
    size_t first_char = value.find_first_not_of(" \t");
    if (std::string::npos == first_char)
    {
        value = ""; // 如果全是空格，则value为空
    }
    else
    {
        value = value.substr(first_char);
    }
    // SPDLOG_DEBUG("parseRequestHeader: Key='{}', Value='{}'", key, value);

    // if (key == "Host")
    // {
    //     headers_[key] = value;
    // }
    // else if (key == "Connection")
    // {
    //     headers_[key] = value;
    // }
    // // 其他头字段也可以类似添加，或者直接添加所有解析到的头字段
    headers_[key] = value; // 直接添加，如果需要特定处理某些头，可以在此基础上修改

    buffer->removeOneLine(); // 这一行处理完了，推进readPos_
    // SPDLOG_DEBUG("Exit parseRequestHeader: Success (processed one header line).");
    return true;
}

bool HttpRequest::parseRequest(std::shared_ptr<Buffer> readBuf, std::shared_ptr<HttpResponse> response,
    std::shared_ptr<Buffer> sendBuf, socket_t socket)
{
    SPDLOG_INFO("begin parseRequest");
    bool flag = true;
    while(flag)
    {
        switch (curState_)
        {
            case ProcessState::ParseReline:
                SPDLOG_INFO("begin parseRequsetLine");
                flag = parseRequsetLine(readBuf);
                break;
            case ProcessState::ParseReHeaders:
                SPDLOG_INFO("begin parseRequestHeader");
                flag = parseRequestHeader(readBuf);
                break;;
            case ProcessState::ParseReBody:
                break;
            default:
                break;
        }
        if(!flag) return flag;
        if(curState_ == ProcessState::ParseReDone)
        {
            //请求报文获取后，去处理这个请求
            SPDLOG_INFO("begin processRequest");
            processRequest(response);
            //处理体内去组织数据
            SPDLOG_INFO("begin prepareMsg");
            response->prepareMsg(sendBuf,socket);
            return true;
        }
    }
    return false;//如果根本没有进入while循环，就返回false
}

bool HttpRequest::processRequest(std::shared_ptr<HttpResponse> response)
{
    if(method_ != "get")
    {
        return false;
    }
    url_ = decodeMsg(url_);
    const char* file = nullptr;
    if(url_ == "/")
    {
        file = "./";
    }
    else
    {
        file = url_.data() + 1;//去掉前面的/，让本机认为是相对路径
    }
    std::filesystem::path filePath(file);
    try {
        // 跨平台的文件状态检查
        if (!std::filesystem::exists(filePath)) {
            // 文件不存在 -- 回复404
            response->setFileName("404.html");
            response->setStatuCode(StatusCode::NotFound);
            response->addHeader("Content-type",getFileType(".html"));
            response->sendDataFunc = sendFile;
            return true;
        }
        response->setFileName(file);
        response->setStatuCode(StatusCode::OK);
        SPDLOG_INFO("set StatusCode:OK");
        if (std::filesystem::is_directory(filePath)) {
            // 处理目录请求（如默认加载index.html）
            response->addHeader("Content-type", getFileType(".html"));
            response->sendDataFunc = sendDir;
            return true;
        }

        //这里是文件类型处理
        auto fileSize = std::filesystem::file_size(filePath); // 获取文件大小
        response->addHeader("Content-type", getFileType(file));
        response->addHeader("Content-length", std::to_string(fileSize));
        response->sendDataFunc = sendFile;
        return true;

    } catch (const std::filesystem::filesystem_error& e) {
        // 文件系统错误处理
        //暂未实现
        return false;
    }
    return false;
}

std::string HttpRequest::getFileType(const std::string& name)
{
    size_t dot = name.rfind('.');
    if (dot == std::string::npos)
    {
        return "text/plain"; // 没后缀，默认纯文本
    }

    std::string ext = name.substr(dot + 1);

    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css")  return "text/css";
    if (ext == "js")   return "application/javascript";
    if (ext == "png")  return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif")  return "image/gif";
    if (ext == "svg")  return "image/svg+xml";
    if (ext == "ico")  return "image/x-icon";
    if (ext == "json") return "application/json";
    if (ext == "pdf")  return "application/pdf";
    if (ext == "zip")  return "application/zip";

    return "application/octet-stream"; // 默认二进制流
}

void HttpRequest::sendDir(std::string dirName, std::shared_ptr<Buffer> sendBuf, socket_t cfd)
{
    // 1. 准备HTML头部
    std::ostringstream oss;
    oss << R"(
<!DOCTYPE html>
<html>
<head>
    <title>Index of )" << dirName << R"(</title>
    <style>
        body { font-family: sans-serif; }
        table { width: 100%; border-collapse: collapse; }
        th, td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background-color: #f2f2f2; }
        a { text-decoration: none; color: #0366d6; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <h1>Index of )" << dirName << R"(</h1>
    <table>
        <tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>
    )";

    // 2. 添加父目录链接（如果不是根目录）
    if (dirName != "./" && dirName != "/") {
        std::filesystem::path parentPath = std::filesystem::path(dirName).parent_path();
        if (parentPath.empty()) parentPath = "./";

        oss << R"(<tr><td><a href=")" << parentPath.string() << R"(/">..</a></td><td>-</td><td>-</td></tr>)";
    }

    // 3. 遍历目录项
    try {
        for (const auto& entry : std::filesystem::directory_iterator(dirName)) {
            const auto& path = entry.path();
            std::string filename = path.filename().string();

            // 跳过 "." 和 ".."
            if (filename == "." || filename == "..") continue;

            // 获取文件信息和最后修改时间
            auto fileTime = std::filesystem::last_write_time(entry);

            // 将文件时间转换为 system_clock::time_point
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
    fileTime - std::filesystem::file_time_type::clock::now()
    + std::chrono::system_clock::now());
            // 转换为 time_t
            std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

            // 本地时间格式化
            std::tm tm = *std::localtime(&cftime);
            char timeBuf[64];
            std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);
            // 生成表格行
            if (entry.is_directory()) {
                oss << R"(<tr><td><a href=")" << filename << R"(/">)" << filename << R"(/</a></td><td>-</td><td>)"
                    << timeBuf << R"(</td></tr>)";
            } else {
                // 格式化文件大小
                auto size = entry.file_size();
                std::string sizeStr;
                if (size < 1024) {
                    sizeStr = std::to_string(size) + " B";
                } else if (size < 1024 * 1024) {
                    sizeStr = std::to_string(size / 1024) + " KB";
                } else {
                    sizeStr = std::to_string(size / (1024 * 1024)) + " MB";
                }

                oss << R"(<tr><td><a href=")" << filename << R"(">)" << filename << R"(</a></td><td>)"
                    << sizeStr << R"(</td><td>)" << timeBuf << R"(</td></tr>)";
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        oss << R"(<tr><td colspan="3">Error: )" << e.what() << R"(</td></tr>)";
    }

    // 4. 添加HTML尾部
    oss << R"(
        </table>
    </body>
</html>
    )";

    // 5. 发送生成的HTML
    sendBuf->appendString(oss.str().c_str());
    sendBuf->sendData(cfd);
}

void HttpRequest::sendFile(std::string fileName, std::shared_ptr<Buffer> sendBuf, socket_t cfd)
{
    std::ifstream file(fileName, std::ios::binary); // 二进制打开
    if (!file.is_open())
    {
        perror("open file failed");
        return;
    }

    constexpr size_t bufSize = 8192;
    std::vector<char> buffer(bufSize);

    while (file)
    {
        file.read(buffer.data(), bufSize);
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0)
        {
            sendBuf->appendString(buffer.data(), static_cast<int>(bytesRead));
            sendBuf->sendData(cfd);
        }
    }

}

std::string HttpRequest::decodeMsg(const std::string& from)
{
    std::string result;
    char ch;
    int i, ii;
    for (i = 0; i < from.length(); i++)
    {
        if (from[i] == '%')
        {
            if (i + 2 < from.length())
            {
                sscanf(from.substr(i + 1, 2).c_str(), "%x", &ii);
                ch = static_cast<char>(ii);
                result += ch;
                i = i + 2;
            }
        }
        else if (from[i] == '+')
        {
            result += ' '; // 有些表单是把空格编码成+的
        }
        else
        {
            result += from[i];
        }
    }
    return result;
}
