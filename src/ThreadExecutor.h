//
// Created by adebayo on 4/4/21.
//

#ifndef LIBMEDIAFYANDROID_THREADEXECUTOR_H
#define LIBMEDIAFYANDROID_THREADEXECUTOR_H
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace libmediafy {

class ThreadExecutor {
public:
    explicit ThreadExecutor(const char* thread_name = "thrd_ex");
    void queue_execute(std::function<void()> func);
    void stop();
    ~ThreadExecutor() { stop(); }
private:
    std::atomic_bool running{false};
    std::thread thread;
    void thread_func();
    std::mutex queue_lock;
    std::condition_variable queue_cond;
    std::queue<std::function<void()>> runnables;
};
}

#endif //LIBMEDIAFYANDROID_THREADEXECUTOR_H
