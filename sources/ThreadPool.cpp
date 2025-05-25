//
// Created by 洛琪希 on 25-5-3.
//

#include "../headers/ThreadPool.h"
#include <cassert>
ThreadPool::ThreadPool(std::shared_ptr<EventLoop> evLoop, int count)
{
    mainLoop_ = evLoop;
    threadNum_ = (count <= 0) ? std::thread::hardware_concurrency() : count;
    isStart_ = false;
    index_ = 0;
}

void ThreadPool::run()
{
    assert(!isStart_);
    if(mainLoop_->getThreadId() != std::this_thread::get_id())
    {
        exit(0);
    }
    if(threadNum_ > 0)
    {
        workerThreads_.reserve(threadNum_);
        for(int i = 0; i < threadNum_; ++i)
        {
            std::shared_ptr<WorkerThread> subThread = std::make_shared<WorkerThread>(i);
            subThread->run();
            workerThreads_.emplace_back(subThread);
        }
        isStart_ = true;
    }
}

std::shared_ptr<EventLoop> ThreadPool::takeWorkerEventLoop()
{
    assert(isStart_);
    if(mainLoop_->getThreadId() != std::this_thread::get_id())
    {
        exit(0);
    }
    
    if(threadNum_ <= 0 || workerThreads_.empty())
    {
        return mainLoop_;
    }
    
    auto evLoop = workerThreads_[index_]->getEvLoop();
    index_ = ++index_ % threadNum_;
    return evLoop;
}
