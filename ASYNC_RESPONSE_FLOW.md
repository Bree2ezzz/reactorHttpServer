# Async Response Flow Overview

This document explains the post-refactor response pipeline for the Linux build, highlights the structural changes that enable non-blocking writes, and walks through the complete request lifecycle so new contributors can reason about the codepath quickly.

## Key Structural Changes

- **HttpResponse (`headers/HttpResponse.h`)**
  - Introduced `ResponseBodyType` enum and fields (`bodyContent_`, `fileSize_`, `shouldClose_`) so responses can describe their payload without sending immediately.
  - Added `setBodyContent()` / `setFileBody()` helpers to capture in-memory payloads vs. file-backed bodies.
  - `prepareMsg()` now only serialises headers/body into the shared `Buffer` on Linux; synchronous sending is confined to the Windows branch.

- **TcpConnection (`headers/TcpConnection.h`)**
  - Tracks async write state via `closeAfterWrite_`, `hasFileBody_`, `fileFd_`, and `fileOffset_`.
  - `processHttpRequest()` inspects the response metadata, primes the write buffer, and opens the target file when needed.
  - `writeCallback()` drains the header buffer and streams file data with `sendfile`, toggling `EPOLLOUT` off once all bytes are flushed.

- **Buffer (`sources/Buffer.cpp`)**
  - `sendData()` recognises `EAGAIN` / `EINTR`, returning early so the caller can retry when epoll reports the socket writable again.

- **TcpSever (`sources/TcpSever.cpp`)**
  - Newly accepted sockets are switched to non-blocking mode to cooperate with edge-triggered epoll and the asynchronous writer.

## Lifecycle Walkthrough

Below is the typical flow from server start-up to completing an HTTP download:

1. **Bootstrap**
   - `TcpSever::setListen()` creates and binds the listening socket.
   - `TcpSever::run()` spins worker threads (`ThreadPool::run()`), adds a read channel for the listener, and starts the main `EventLoop`.

2. **Accept**
   - When the OS reports the listener readable, `TcpSever::readCallback()` accepts the connection, sets `O_NONBLOCK`, and instantiates `TcpConnection`.
   - `TcpConnection::init()` creates a `Channel` bound to the connection fd, registers it with the worker loop, and (Windows only) issues the first asynchronous read.

3. **Read & Parse**
   - For Linux, `TcpConnection::readCallback()` reads repeatedly into `readBuf_` until `recv()` returns `EAGAIN`, then calls `processHttpRequest()`.
   - `TcpConnection::processHttpRequest()` clears previous state, then delegates to `HttpRequest::parseRequest()`.
   - `HttpRequest::parseRequest()` drives the state machine (`ProcessState`), and once headers are parsed, invokes `processRequest()` to build a response.

4. **Response Assembly**
   - `HttpRequest::processRequest()` inspects the URL, resolves the filesystem path, and prepares `HttpResponse`.
     - Directories: generate HTML with `generateDirHTML()`, store via `setBodyContent()`, and set headers (content-type, length, `Connection: close`).
     - Files: call `setFileBody()` with the resolved path and size, forcing download headers and marking the connection for closure.
     - Missing entries: fall back to `404.html` (if present) or an in-memory HTML snippet.
   - `HttpResponse::prepareMsg()` serialises the status line and headers into `writeBuf_`; in-memory bodies are appended immediately, file bodies are deferred.

5. **Schedule Write**
   - Back in `TcpConnection::processHttpRequest()`, the code:
     - Resets/opens file descriptors when `ResponseBodyType::File` is selected.
     - Persists `fileSize_`, `fileOffset_`, and `closeAfterWrite_`.
     - Enables the channel’s write interest and queues an `ETMODIFY` task so epoll starts reporting `EPOLLOUT`.

6. **Non-blocking Flush**
   - `EventLoop::eventActive()` dispatches the writable event to `TcpConnection::writeCallback()`.
   - `writeCallback()` first drains `writeBuf_` via `Buffer::sendData()`; partial writes simply break and wait for the next `EPOLLOUT`.
   - When the buffer is empty and a file body remains, `sendfile()` streams the next chunk directly from kernel to socket. `fileOffset_` captures progress.
   - Once both header buffer and file transfer are complete, `EPOLLOUT` is disabled. If `closeAfterWrite_` is true, the fd is removed from epoll and closed.

7. **Connection Teardown**
   - Short-lived transfers close immediately. If you later enable keep-alive, you can flip `shouldClose_` to false and avoid `ETDELETE`, keeping the channel read-ready for the next request.

## How This Differs From the Previous Design

- Previously, `HttpResponse::prepareMsg()` and `sendFile()` issued blocking `send()` calls, making every response synchronous and tightly coupled to parsing.
- Response metadata was implicit; there was no way to determine whether a file needed to be streamed or if the connection should be preserved.
- The current design splits responsibilities cleanly:
  - **HttpRequest/HttpResponse** build a description of what should be sent.
  - **TcpConnection** owns the mechanics of flushing that description asynchronously, using zero-copy (`sendfile`) when serving files.
  - **EventLoop** only reacts to readiness notifications, keeping worker threads responsive under load.

Armed with this sequence and the new member fields, you can inspect each component in isolation or trace logs (`spdlog` spans every phase) to validate behaviour end-to-end.

