//
// Created by adebayo on 4/4/21.
//

#include "ThreadExecutor.h"

extern "C" {
#include <libavutil/time.h>
}

#include <pthread.h>

libmediafy::ThreadExecutor::ThreadExecutor(const char* thread_name) {
    running = true;
    thread = std::thread(&ThreadExecutor::thread_func, this);
    pthread_setname_np(thread.native_handle(), thread_name);
}

void libmediafy::ThreadExecutor::thread_func() {
    while (running) {
        std::function<void()> runnable{nullptr};
        {
            std::unique_lock<std::mutex> lock{this->queue_lock};
            while (runnables.empty()) {
                queue_cond.wait(lock);
                if (!running) return;
            }
            runnable = runnables.front();
            runnables.pop();
        }
        if (runnable) {
            runnable();
        }
    }
}

void libmediafy::ThreadExecutor::queue_execute(std::function<void()> func) {
    if (!running) return;
    std::unique_lock<std::mutex> lock{this->queue_lock};
    runnables.push(func);
    queue_cond.notify_one();
}

void libmediafy::ThreadExecutor::stop() {
    running = false;
    queue_cond.notify_one();
    if (thread.joinable()) thread.join();

    while (!runnables.empty()) {
        runnables.pop();
    }
}
