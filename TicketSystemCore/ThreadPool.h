#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

using namespace std;

//统一处理线程调度任务
class ThreadPool {

private:

    vector<thread> workers;

    queue<function<void()>> tasks;

    mutex queueMutex;

    condition_variable condition;

    bool stopping;

    condition_variable finishedCondition;

    int activeTasks;

private:

    void workerLoop();

public:

    ThreadPool(int threadCount);

    ~ThreadPool();

    bool submit(function<void()> task);

    void waitUntilFinished();
};