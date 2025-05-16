cmake_minimum_required(VERSION 3.10)

project(spdlog-header-only LANGUAGES CXX)

add_library(spdlog INTERFACE)

target_include_directories(spdlog INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 可选：让其他地方识别 spdlog::spdlog_header_only 这种别名
add_library(spdlog::spdlog ALIAS spdlog)
add_library(spdlog::spdlog_header_only ALIAS spdlog)
