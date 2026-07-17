#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "concurrency/ICommand.hpp"

// Thread-safe command queue: any thread may post(); a dedicated dispatcher
// thread executes commands against Target (single consumer / single writer).
template <typename Target>
class QueuedCommandExecutor {
public:
    explicit QueuedCommandExecutor(Target& target) : target_(target), dispatcher_([this]() { dispatcher_loop(); }) {}

    ~QueuedCommandExecutor() { stop(); }

    QueuedCommandExecutor(const QueuedCommandExecutor&) = delete;
    QueuedCommandExecutor& operator=(const QueuedCommandExecutor&) = delete;

    void post(std::unique_ptr<ICommand<Target>> command) {
        if (!command) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) {
                return;
            }
            queue_.push_back(std::move(command));
        }
        cv_.notify_one();
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
        cv_.notify_all();
        if (dispatcher_.joinable()) {
            dispatcher_.join();
        }
    }

private:
    void dispatcher_loop() {
        while (true) {
            std::unique_ptr<ICommand<Target>> command;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this]() { return !queue_.empty() || !running_; });
                if (queue_.empty()) {
                    return;
                }
                command = std::move(queue_.front());
                queue_.pop_front();
            }
            command->execute(target_);
        }
    }

    Target& target_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::unique_ptr<ICommand<Target>>> queue_;
    std::atomic<bool> running_{true};
    std::thread dispatcher_;
};
