//
// Created by adebayo on 5/7/21.
//

#ifndef LIBMEDIAFYANDROID_TSQUEUE_H
#define LIBMEDIAFYANDROID_TSQUEUE_H

#include <deque>
#include <mutex>

namespace libmediafy {
    template<typename T>
    class TSQueue {
    public:
        TSQueue(unsigned int capacity) : capacity(capacity) {}
        unsigned int get_capacity() {
            std::unique_lock<std::mutex> lock1{lock};
            return capacity;
        }
        int size() {
            std::unique_lock<std::mutex> lock1{lock};
            return queue.size();
        }
        bool empty() {
            std::unique_lock<std::mutex> lock1{lock};
            return queue.empty();
        }
        bool enqueue(T object) {
            std::unique_lock<std::mutex> lock1{lock};
            if (queue.size() >= capacity) return false;
            queue.push_back(object);
            return true;
        }
        T dequeue() {
            std::unique_lock<std::mutex> lock1{lock};
            T t = queue.front();
            queue.pop_front();
            return t;
        }
        T peek() {
            std::unique_lock<std::mutex> lock1{lock};
            return queue.front();
        }

        void remove_front() {
            std::unique_lock<std::mutex> lock1{lock};
            queue.pop_front();
        }

        void clear() {
            std::unique_lock<std::mutex> lock1{lock};
            queue.clear();
        }

        void for_each(std::function<void(T&)> func) {
            std::unique_lock<std::mutex> lock1{lock};
            std::for_each(queue.begin(), queue.end(), func);
        }
    private:
        unsigned int capacity;
        std::mutex lock{};
        std::deque<T> queue;
    };
}

#endif //LIBMEDIAFYANDROID_TSQUEUE_H
