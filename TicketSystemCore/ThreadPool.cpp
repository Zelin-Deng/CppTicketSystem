#include "ThreadPool.h"


ThreadPool::ThreadPool(int threadCount)
    : stopping(false),
      activeTasks(0)
{
    for (int i = 0; i < threadCount; i++)
    {
        workers.emplace_back(
            &ThreadPool::workerLoop,
            this
        );
    }
}

ThreadPool::~ThreadPool() {

    // 先等待已经提交的任务执行完成
    waitUntilFinished();

    {
        lock_guard<mutex> lock(queueMutex);

        stopping = true;
    }

    condition.notify_all();

    for (auto& worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

void ThreadPool::workerLoop() {
    while (true)
    {
        function<void()> task;

        {
            unique_lock<mutex> lock(queueMutex);

            condition.wait(
                lock,
                [this]()
                {
                    return stopping || !tasks.empty();
                }
            );

            if (stopping && tasks.empty())
            {
                return;
            }

            task = move(tasks.front());

            tasks.pop();

            activeTasks++;
        }

        task();

        {
            lock_guard<mutex> lock(queueMutex);

            // 当前任务执行结束
            activeTasks--;

            // 队列没有等待任务
            // 并且也没有worker正在执行任务
            if (tasks.empty() && activeTasks == 0)
            {
                finishedCondition.notify_all();
            }
        }
    }
}

bool ThreadPool::submit(function<void()> task) {

    lock_guard<mutex> lock(queueMutex);
    {
        if (stopping)
        {
            return false;
        }

        tasks.push(move(task));
    }

    //唤醒一个线程处理任务
    condition.notify_one();

    return true;
}

void ThreadPool::waitUntilFinished()
{
    unique_lock<mutex> lock(queueMutex);

    finishedCondition.wait(
        lock,
        [this]()
        {
            return tasks.empty()
                && activeTasks == 0;
        }
    );
}