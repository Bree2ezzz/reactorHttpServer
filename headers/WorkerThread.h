//
// Created by 洛琪希 on 25-5-2.
//

#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H
#include <thread>
#include "EventLoop.h"
#include <condition_variable>

class WorkerThread {

public:
    WorkerThread(int index);
    ~WorkerThread() = default;
    void run();
    std::shared_ptr<EventLoop> getEvLoop();

private:
    std::shared_ptr<std::thread> thread_;
    std::thread::id threadId_;
    std::string threadName_;
    std::mutex threadMutex_;
    std::condition_variable threadCond_;
    std::shared_ptr<EventLoop> evLoop_;

};



#endif //WORKERTHREAD_H
