//
// Created by 洛琪希 on 25-5-3.
//

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "EventLoop.h"
#include "WorkerThread.h"
class ThreadPool {

public:
    ThreadPool(std::shared_ptr<EventLoop> evLoop,int count);
    ~ThreadPool() = default;
    void run();//启动
    std::shared_ptr<EventLoop> takeWorkerEventLoop();
private:
    std::shared_ptr<EventLoop> mainLoop_;//主线程的反应堆实例
    bool isStart_;
    int threadNum_;
    std::vector<std::shared_ptr<WorkerThread>> workerThreads_;
    int index_;//控制取哪一个线程
};



#endif //THREADPOOL_H
