#include "resultThreadPool.h"
#include <cxxabi.h>

ResultThreadPool::ResultThreadPool(int maxCount) : stop(false) {
    for (int i = 0; i < maxCount; ++i) {
        workers.emplace_back([this, i]() {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(mutex);
                    condition.wait(lock, [this]() {
                        return stop || !tasks.empty();
                    });

                    if (stop && tasks.empty())
                        return;

                    task = std::move(tasks.front());
                    tasks.pop();
                }
                try {
                    task();
                } catch (std::exception &e) {
                    // Analize how to push exceptions back to the original thread
                }
            }
        });
    }
}

ResultThreadPool::~ResultThreadPool() {
    stop = true;
    condition.notify_all();
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }
}

void ResultThreadPool::enqueue(const std::function<void()>& task) {
    {
        std::unique_lock<std::mutex> lock(mutex);
        tasks.push(task);
    }
    condition.notify_one();
}

int ResultThreadPool::getThreadCount() {
    return workers.size();
}

std::string ResultThreadPool::demangle(const char* name) {
    int status = 0;
    char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    std::string result = (status == 0 && demangled != nullptr) ? demangled : name;
    free(demangled);
    return result;
}