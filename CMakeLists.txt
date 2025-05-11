cmake_minimum_required(VERSION 3.28)
project(reactorHttp)

set(CMAKE_CXX_STANDARD 17)

#公共源文件
set(PUBLIC_SOURCES main.cpp
        sources/Channel.cpp
        headers/Channel.h
        sources/Dispatcher.cpp
        headers/Dispatcher.h
        sources/EventLoop.cpp
        headers/EventLoop.h
        sources/EpollDispatcher.cpp
        headers/EpollDispatcher.h
        sources/WorkerThread.cpp
        headers/WorkerThread.h
        sources/ThreadPool.cpp
        headers/ThreadPool.h
        sources/TcpSever.cpp
        headers/TcpSever.h
        sources/Buffer.cpp
        headers/Buffer.h
        sources/HttpRequest.cpp
        headers/HttpRequest.h
        sources/HttpResponse.cpp
        headers/HttpResponse.h
        sources/TcpConnection.cpp
        headers/TcpConnection.h)
#if(WIN32)
#{
#    list(APPEND SOURCES)
#}
#else
#{
#
#}
#endif
add_executable(${PROJECT_NAME} ${PUBLIC_SOURCES})
if(WIN32)
target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()