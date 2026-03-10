#pragma once

#include <windows.h>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

class AsyncTaskQueue {
public:
    static constexpr UINT WM_ASYNC_UI_TASK = WM_APP + 42;

    AsyncTaskQueue()
        : stopRequested(false)
        , workerThread([this]() { WorkerLoop(); }) {
    }

    ~AsyncTaskQueue() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stopRequested = true;
        }
        queueCondition.notify_all();
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    AsyncTaskQueue(const AsyncTaskQueue&) = delete;
    AsyncTaskQueue& operator=(const AsyncTaskQueue&) = delete;

    void PostWorkerTask(std::function<void()> task) {
        if (!task) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            taskQueue.push(std::move(task));
        }
        queueCondition.notify_one();
    }

    static void PostUiTask(HWND hWnd, std::function<void()> task) {
        if (!hWnd || !task) {
            return;
        }

        std::function<void()>* heapTask = new std::function<void()>(std::move(task));
        if (!PostMessage(hWnd, WM_ASYNC_UI_TASK, 0, reinterpret_cast<LPARAM>(heapTask))) {
            delete heapTask;
        }
    }

    static bool TryDispatchUiTask(UINT message, LPARAM lParam) {
        if (message != WM_ASYNC_UI_TASK) {
            return false;
        }

        std::function<void()>* heapTask = reinterpret_cast<std::function<void()>*>(lParam);
        if (!heapTask) {
            return true;
        }

        std::unique_ptr<std::function<void()>> ownedTask(heapTask);
        (*ownedTask)();
        return true;
    }

private:
    void WorkerLoop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                queueCondition.wait(lock, [this]() {
                    return stopRequested || !taskQueue.empty();
                });

                if (stopRequested && taskQueue.empty()) {
                    return;
                }

                task = std::move(taskQueue.front());
                taskQueue.pop();
            }

            if (task) {
                task();
            }
        }
    }

    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::queue<std::function<void()>> taskQueue;
    bool stopRequested;
    std::thread workerThread;
};
