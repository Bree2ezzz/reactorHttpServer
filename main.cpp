#include <iostream>
#include "headers/Channel.h"
#include "headers/TcpSever.h"

int main(int argc,char* argv[])
{
    unsigned short port = 20500;
    chdir("/home/roxy/wangyeceshi");
    std::shared_ptr<TcpSever> server = std::make_shared<TcpSever>(port,4);
    server->setListen();
    server->run();
    return 0;
}
