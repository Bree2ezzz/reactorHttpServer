//
// Created by 洛琪希 on 25-5-7.
//

#include "../headers/HttpRequest.h"
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <system_error>
#include <spdlog/spdlog.h>
#include <algorithm> 
#include <cstddef>

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
        SPDLOG_INFO("parseRequsetLine: No space found for METHOD.");
        return false;
    }
    method_.assign(start, space);
    SPDLOG_INFO("parseRequsetLine: Method: {}", method_);

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
        SPDLOG_INFO("parseRequsetLine: versionStart is out of bounds after second space.");
        return false;
    }
    version_.assign(versionStart, end);

    buffer->removeOneLine();
    curState_ = ProcessState::ParseReHeaders;
    SPDLOG_INFO("Exit parseRequsetLine: Success.");
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
        SPDLOG_INFO("parseRequestHeader: Empty line found. Headers done.");
        return true;
    }

    char* colon = std::find(lineStart, crlf, ':');
    if (colon == crlf || lineStart >= colon) // 检查是否有冒号，并且key不为空
    {
        SPDLOG_INFO("parseRequestHeader: No colon found, or key is empty.");
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
    if(method_ != "GET")
    {
        return false;
    }
    // 解码URL
    url_ = decodeMsg(url_);
    
    // 处理URL中的".."返回上一级目录
    if (url_.find("..") != std::string::npos) {
        // 对于包含".."的URL，我们需要特殊处理
        SPDLOG_INFO("处理包含'..'的URL: {}", url_);
        
        try {
            // 转换为文件系统路径并处理
            std::string path = url_.substr(1);  // 去掉开头的'/'
            if (path.empty()) path = ".";
            
            // 处理相对路径
            std::filesystem::path requestPath(path);
            
            // 使用std::filesystem来解析路径
            std::filesystem::path resolvedPath;
            if (requestPath.is_absolute()) {
                // 安全检查：确保不会访问根目录以外的内容
                resolvedPath = requestPath;
            } else {
                // 相对路径：从当前工作目录解析
                resolvedPath = std::filesystem::weakly_canonical("." / requestPath);
            }
            
            // 将解析后的路径转换回URL格式
            std::string newUrl = "/" + resolvedPath.relative_path().string();
            if (newUrl.empty()) newUrl = "/";
            
            SPDLOG_INFO("解析后的URL: {}", newUrl);
            url_ = newUrl;
        } catch (const std::filesystem::filesystem_error& e) {
            // 路径解析错误，返回404
            SPDLOG_ERROR("路径解析错误: {}", e.what());
            response->setFileName("404.html");
            response->setStatuCode(StatusCode::NotFound);
            response->addHeader("Content-type", getFileType(".html"));
            response->sendDataFunc = sendFile;
            return true;
        }
    }
    
    // 转换URL到文件路径
    std::string file;
    if(url_ == "/")
    {
        file = "./";
    }
    else
    {
        file = url_.substr(1);
#ifdef _WIN32
        // Windows下将正斜杠替换为反斜杠
        std::replace(file.begin(), file.end(), '/', '\\');
#endif
    }
    
    // 记录请求的文件路径，便于调试
    SPDLOG_INFO("请求文件路径: {}", file);
    
    std::filesystem::path filePath(file);
    try {

        SPDLOG_INFO("absolute path: {}", std::filesystem::absolute(filePath).string());
        
        // 跨平台的文件状态检查
        bool exists = std::filesystem::exists(filePath);
        SPDLOG_INFO("文件存在检查结果: {}", exists);
        
        if (!exists) {
            // 文件不存在 -- 回复404
            response->setFileName("404.html");
            response->setStatuCode(StatusCode::NotFound);
            response->addHeader("Content-type", getFileType(".html"));
            response->sendDataFunc = sendFile;
            SPDLOG_WARN("file does not exist: {}", file);
            return true;
        }
        
        // 设置要处理的文件名
        response->setFileName(file.c_str());
        response->setStatuCode(StatusCode::OK);
        SPDLOG_INFO("set StatuCode: OK, fileName: {}", file);
        
        if (std::filesystem::is_directory(filePath)) {
            // 处理目录请求 - 先生成HTML内容来获取长度
            SPDLOG_INFO("process dir request: {}", file);
            
            // 先生成HTML内容来获取长度
            std::string htmlContent = generateDirHTML(file);
            
            response->addHeader("Content-type", getFileType(".html"));
            response->addHeader("Content-Length", std::to_string(htmlContent.length()));
            response->addHeader("Connection", "close");
            response->sendDataFunc = sendDir;
            return true;
        }

        // 处理文件请求 - 强制下载
        SPDLOG_INFO("process file request: {}", file);
        auto fileSize = std::filesystem::file_size(filePath);
        response->addHeader("Content-type", "application/octet-stream"); // 强制所有文件都用二进制流处理，浏览器会下载
        response->addHeader("Content-length", std::to_string(fileSize));
        // 添加Content-Disposition头，强制下载
        std::string filename = std::filesystem::path(file).filename().string();
        response->addHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
        response->sendDataFunc = sendFile;
        return true;

    } catch (const std::filesystem::filesystem_error& e) {
        // 文件系统错误处理
        SPDLOG_ERROR("filesystem error: {}", e.what());
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
    // 生成HTML内容并发送（不包括HTTP头，头部由prepareMsg发送）
    std::string htmlContent = generateDirHTML(dirName);
    sendBuf->appendString(htmlContent.c_str());
    sendBuf->sendData(cfd);
    
    // 确保数据完全发送
    while (sendBuf->readableSize() > 0) {
        int sent = sendBuf->sendData(cfd);
        if (sent <= 0) break;
    }
}

void HttpRequest::sendFile(std::string fileName, std::shared_ptr<Buffer> sendBuf, socket_t cfd)
{
    // 重置缓冲区，准备发送新文件
    sendBuf->reset();
    
    std::ifstream file(fileName, std::ios::binary); // 二进制打开
    if (!file.is_open())
    {
        perror("open file failed");
        return;
    }

    constexpr size_t bufSize = 8192;
    std::vector<char> buffer(bufSize);

    bool fileEnd = false;
    while (!fileEnd)
    {
        // 读取文件数据
        file.read(buffer.data(), bufSize);
        std::streamsize bytesRead = file.gcount();
        
        // 检查是否到达文件末尾
        fileEnd = file.eof();
        
        if (bytesRead > 0)
        {
            // 添加数据到发送缓冲区
            sendBuf->appendString(buffer.data(), static_cast<int>(bytesRead));
            
            // 确保完全发送当前缓冲区数据
            int remainingBytes = sendBuf->readableSize();
            while (remainingBytes > 0)
            {
                int sent = sendBuf->sendData(cfd);
                if (sent <= 0)
                {
                    // 发送失败，可能是连接断开
                    SPDLOG_ERROR("send file data failed: {}", fileName);
                    file.close();
                    return;
                }
                remainingBytes = sendBuf->readableSize();
            }
        }
    }
    
    // 确保文件被关闭
    file.close();
    SPDLOG_INFO("file send done: {}", fileName);
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
            result += ' ';
        }
        else
        {
            result += from[i];
        }
    }
    return result;
}

std::string HttpRequest::generateDirHTML(const std::string& dirName)
{
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
        // 最简单的方式：直接链接到".."
        std::string currentPath = "/" + std::filesystem::path(dirName).string();
        // 确保URL使用正斜杠（无论在哪个平台）
        std::replace(currentPath.begin(), currentPath.end(), '\\', '/');
        if (currentPath.back() != '/') {
            currentPath += "/";
        }
        
        // 上级目录的链接就是当前路径加上".."
        oss << R"(<tr><td><a href=")" << currentPath << R"(../">..</a></td><td>-</td><td>-</td></tr>)";
    }

    // 3. 遍历目录项 
    try {
        // 获取当前目录的URL路径
        std::string currentPath = "/" + std::filesystem::path(dirName).string();
        // 确保URL使用正斜杠（无论在哪个平台）
        std::replace(currentPath.begin(), currentPath.end(), '\\', '/');
        if (currentPath.back() != '/') {
            currentPath += "/";
        }
        
        // 收集目录项信息，减少文件系统调用
        std::vector<std::tuple<std::string, bool, std::uintmax_t, std::string>> entries;
        
        for (const auto& entry : std::filesystem::directory_iterator(dirName)) {
            const auto& path = entry.path();
            std::string filename = path.filename().string();

            // 跳过 "." 和 ".."
            if (filename == "." || filename == "..") continue;

            bool isDir = false;
            std::uintmax_t size = 0;
            std::string timeStr = "-";
            
            try {
                std::error_code ec;
                isDir = entry.is_directory(ec);
                
                if (!ec && !isDir) {
                    // 只对文件获取大小，目录跳过
                    size = entry.file_size(ec);
                    if (ec) size = 0; // 如果获取失败，设为0
                }
                
#ifdef _WIN32
                // Windows下搞这个有点慢 不做
                timeStr = "-";
#else
                // Linux下正常获取时间
                auto fileTime = std::filesystem::last_write_time(entry, ec);
                if (!ec) {
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        fileTime - std::filesystem::file_time_type::clock::now()
                        + std::chrono::system_clock::now());
                    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
                    std::tm tm = *std::localtime(&cftime);
                    char timeBuf[64];
                    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);
                    timeStr = timeBuf;
                }
#endif
            } catch (...) {
                // 如果出现任何异常，使用默认值
                isDir = false;
                size = 0;
                timeStr = "-";
            }
            
            entries.emplace_back(filename, isDir, size, timeStr);
        }
        
        // 排序：目录在前，然后按名称排序
        std::sort(entries.begin(), entries.end(), 
                  [](const auto& a, const auto& b) {
                      bool aIsDir = std::get<1>(a);
                      bool bIsDir = std::get<1>(b);
                      if (aIsDir != bIsDir) return aIsDir > bIsDir; // 目录在前
                      return std::get<0>(a) < std::get<0>(b); // 按名称排序
                  });
        
        // 生成HTML
        for (const auto& [filename, isDir, size, timeStr] : entries) {
            if (isDir) {
                std::string dirUrl = currentPath + filename + "/";
                oss << R"(<tr><td><a href=")" << dirUrl << R"(">)" << filename << R"(/</a></td><td>-</td><td>)"
                    << timeStr << R"(</td></tr>)";
            } else {
                // 格式化文件大小
                std::string sizeStr;
                if (size == 0) {
                    sizeStr = "0 B";
                } else if (size < 1024) {
                    sizeStr = std::to_string(size) + " B";
                } else if (size < 1024 * 1024) {
                    sizeStr = std::to_string(size / 1024) + " KB";
                } else {
                    sizeStr = std::to_string(size / (1024 * 1024)) + " MB";
                }

                std::string fileUrl = currentPath + filename;
                oss << R"(<tr><td><a href=")" << fileUrl << R"(">)" << filename << R"(</a></td><td>)"
                    << sizeStr << R"(</td><td>)" << timeStr << R"(</td></tr>)";
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

    return oss.str();
}
