cmake_minimum_required(VERSION 3.28)
project(reactorHttp)
set(CMAKE_CXX_STANDARD 17)

# 设置编译优化选项
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
# 开启链接时优化（LTO）
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
# 如果支持，开启更现代的CPU指令集
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

set(SPDLOG_HEADER_ONLY ON)

add_subdirectory(third_party/spdlog)


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
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog_header_only)
if(WIN32)
target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

# 添加一个优化的构建目标
add_custom_target(release
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}
    COMMENT "Building optimized release version")