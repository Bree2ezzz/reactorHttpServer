# ReactorHttp - 跨平台高性能Web服务器

## 项目简介

ReactorHttp是一个基于Reactor模式的高性能HTTP文件服务器，采用C++17开发，支持Linux和Windows双平台。项目实现了完整的HTTP协议解析、文件传输和目录浏览功能，具有优秀的并发性能和跨平台兼容性。

## 核心技术栈

### 网络编程
- **Linux**: Epoll I/O多路复用
- **Windows**: IOCP (I/O Completion Port) 异步I/O
- **跨平台抽象**: 统一的Dispatcher接口设计

### 并发模型
- **Reactor模式**: 事件驱动的网络编程模型
- **多线程**: 主线程负责连接接受，工作线程处理I/O事件
- **线程池**: 高效的工作线程管理

### HTTP协议
- **HTTP/1.1**: 完整的请求解析和响应生成
- **文件传输**: 支持大文件的分块传输
- **目录浏览**: 自动生成美观的HTML目录列表

### 开发工具
- **构建系统**: CMake跨平台构建
- **日志系统**: spdlog高性能日志库
- **编译器**: 支持GCC、Clang、MSVC

## 主要功能特性

### 🚀 高性能
- 基于事件驱动的异步I/O模型
- 零拷贝文件传输优化
- 高效的内存缓冲区管理
- 支持大文件(GB级)下载

### 🌐 跨平台支持
- Linux下使用Epoll实现高并发
- Windows下使用IOCP实现异步I/O
- 统一的API接口，无需修改业务代码

### 📁 文件服务
- 静态文件服务器功能
- 目录浏览和导航
- 自动文件类型识别
- 强制下载模式(所有文件作为附件下载)

### 🔧 易于使用
- 简单的配置和部署
- 自动端口绑定
- 灵活的工作目录设置
- 详细的日志输出

## 项目架构

```
┌─────────────────┐    ┌─────────────────┐
│   TcpServer     │    │   ThreadPool    │
│   (主线程)       │────│   (工作线程池)   │
└─────────────────┘    └─────────────────┘
         │                       │
         ▼                       ▼
┌─────────────────┐    ┌─────────────────┐
│   EventLoop     │    │  WorkerThread   │
│   (事件循环)     │    │  (工作线程)     │
└─────────────────┘    └─────────────────┘
         │                       │
         ▼                       ▼
┌─────────────────┐    ┌─────────────────┐
│   Dispatcher    │    │ TcpConnection   │
│ (I/O多路复用)    │    │  (连接处理)     │
└─────────────────┘    └─────────────────┘
    │           │               │
    ▼           ▼               ▼
┌─────────┐ ┌─────────┐ ┌─────────────────┐
│ Epoll   │ │  IOCP   │ │  HttpRequest/   │
│(Linux)  │ │(Windows)│ │  HttpResponse   │
└─────────┘ └─────────┘ └─────────────────┘
```

### 核心组件说明

- **TcpServer**: 服务器主类，负责监听端口和接受连接
- **EventLoop**: 事件循环，处理I/O事件分发
- **Dispatcher**: I/O多路复用抽象层，支持Epoll和IOCP
- **ThreadPool**: 线程池管理，提供工作线程
- **TcpConnection**: TCP连接封装，处理单个客户端连接
- **HttpRequest/Response**: HTTP协议解析和响应生成
- **Buffer**: 高效的网络缓冲区实现

## 编译和运行

### 环境要求
- C++17兼容编译器
- CMake 3.28+
- Linux: GCC 7+ 或 Clang 5+
- Windows: Visual Studio 2019+ 或 MinGW-w64

### 编译步骤

```bash
# 克隆项目
git clone
cd reactorHttp

# 创建构建目录
mkdir build && cd build

# 配置和编译
cmake ../
make
```

### 运行服务器

```bash
# Linux
./reactorHttp

# Windows
reactorHttp.exe
```

服务器默认监听端口20500，可通过浏览器访问 `http://localhost:20500`

### 配置说明

在`main.cpp`中可以修改以下配置：

```cpp
unsigned short port = 20500;        // 监听端口
int threadNum = 4;                   // 工作线程数

// 工作目录设置
#ifdef _WIN32
    _chdir("D:\\wangyeceshi");       // Windows路径
#else
    chdir("/home/roxy/wangyeceshi"); // Linux路径
#endif
```

## 性能特点

### 并发能力
- 支持数千个并发连接
- 事件驱动模型，低CPU占用
- 内存使用优化，支持大文件传输

### 传输性能
- 零拷贝优化的文件传输
- 分块传输，支持GB级大文件
- 自适应缓冲区大小

### 跨平台性能
- Linux: 利用Epoll的边缘触发模式
- Windows: 充分发挥IOCP的异步优势
- 统一的性能调优接口

## 使用场景

- **文件共享服务器**: 局域网文件共享
- **静态资源服务**: Web应用的静态资源托管
- **开发测试**: 本地开发环境的文件服务
- **学习研究**: 网络编程和HTTP协议学习

## 技术亮点

1. **跨平台I/O抽象**: 通过Dispatcher接口统一Linux Epoll和Windows IOCP
2. **工厂模式**: DispatcherFactory根据平台自动选择合适的I/O模型
3. **RAII设计**: 智能指针管理资源，避免内存泄漏
4. **事件驱动**: 高效的Reactor模式实现
5. **模块化设计**: 清晰的组件分离，易于维护和扩展

---

**注意**: 本项目主要用于学习和研究目的，生产环境使用请进行充分测试。 

