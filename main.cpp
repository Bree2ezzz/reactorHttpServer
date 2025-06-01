#include <iostream>
#include "headers/Channel.h"
#include "headers/TcpSever.h"
#include <spdlog/spdlog.h>
#ifdef _WIN32
#include <direct.h> // For _chdir on Windows
#else
#include <unistd.h> // For chdir on Linux
#endif

int main(int argc, char* argv[])
{
    spdlog::set_pattern("[%s:%#] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::info);
    
    unsigned short port = 20500;
    
#ifdef _WIN32
    // Windows平台下的工作目录设置
    // 可以根据需要修改或使用相对路径
    // 注意：Windows下不能使用chdir函数设置到Linux路径
    _chdir("D:\\wangyeceshi");
#else
    // Linux平台下的工作目录设置
    chdir("/home/roxy/wangyeceshi");
#endif
    
    std::shared_ptr<TcpSever> server = std::make_shared<TcpSever>(port, 4);
    server->setListen();
    SPDLOG_INFO("server set listen port:{}", port);
    server->run();
    SPDLOG_INFO("server run");
    return 0;
}
