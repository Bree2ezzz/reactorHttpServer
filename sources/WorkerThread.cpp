//
// Created by 洛琪希 on 25-5-2.
//

#include "../headers/WorkerThread.h"

WorkerThread::WorkerThread(int index)
{
    evLoop_ = nullptr;
    thread_ = nullptr;
    threadId_ = std::thread::id();
    threadName_ = "SubThread" + std::to_string(index);
}

void WorkerThread::run()
{
    thread_ = std::make_shared<std::thread>([this]() {
        {
            std::lock_guard<std::mutex> mtx(threadMutex_);
            evLoop_ = std::make_shared<EventLoop>(threadName_);
        }
        threadCond_.notify_one();
        evLoop_->run();
    });
    std::unique_lock<std::mutex> mtx(threadMutex_);
    while(evLoop_ == nullptr)
    {
        threadCond_.wait(mtx);
    }
}

std::shared_ptr<EventLoop> WorkerThread::getEvLoop()
{
    return evLoop_;
}
