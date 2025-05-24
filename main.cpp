#include <iostream>
#include "headers/Channel.h"
#include "headers/TcpSever.h"
#include <spdlog/spdlog.h>
int main(int argc,char* argv[])
{
    spdlog::set_pattern("[%s:%#] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::warn);
    unsigned short port = 20500;
    chdir("/home/roxy/wangyeceshi");
    std::shared_ptr<TcpSever> server = std::make_shared<TcpSever>(port,4);
    server->setListen();
    SPDLOG_INFO("server set listen port:{}",port);
    server->run();
    SPDLOG_INFO("server run");
    return 0;
}
