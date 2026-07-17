#include "logging/FileLogger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

namespace logging {

namespace {

std::string utc_timestamp() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&t, &tm);
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

}  // namespace

FileLogger::FileLogger(std::string path) {
    open(std::move(path));
}

FileLogger::~FileLogger() {
    stop();
}

void FileLogger::open(std::string path) {
    stop();

    std::lock_guard<std::mutex> lock(mutex_);
    path_ = std::move(path);
    stream_.open(path_, std::ios::out | std::ios::app);
    open_ = stream_.is_open();
    if (!open_) {
        return;
    }
    running_ = true;
    writer_ = std::thread([this]() { writer_loop(); });
}

bool FileLogger::is_open() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return open_;
}

void FileLogger::info(const std::string& message) {
    enqueue("INFO", message);
}

void FileLogger::warn(const std::string& message) {
    enqueue("WARN", message);
}

void FileLogger::error(const std::string& message) {
    enqueue("ERROR", message);
}

void FileLogger::enqueue(const char* level, std::string message) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ || !open_) {
            return;
        }
        queue_.push_back(Entry{level, std::move(message)});
    }
    cv_has_work_.notify_one();
}

void FileLogger::flush() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_drained_.wait(lock, [this]() { return queue_.empty(); });
}

void FileLogger::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ && !writer_.joinable()) {
            return;
        }
        running_ = false;
    }
    cv_has_work_.notify_all();
    if (writer_.joinable()) {
        writer_.join();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (stream_.is_open()) {
        stream_.flush();
        stream_.close();
    }
    open_ = false;
    queue_.clear();
}

void FileLogger::writer_loop() {
    while (true) {
        Entry entry;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_has_work_.wait(lock, [this]() { return !queue_.empty() || !running_; });
            if (queue_.empty()) {
                cv_drained_.notify_all();
                if (!running_) {
                    return;
                }
                continue;
            }
            entry = std::move(queue_.front());
            queue_.pop_front();
            if (queue_.empty()) {
                cv_drained_.notify_all();
            }
        }
        write_entry(entry);
    }
}

void FileLogger::write_entry(const Entry& entry) {
    // Single writer thread: no concurrent file writes.
    if (!stream_.is_open()) {
        return;
    }
    stream_ << utc_timestamp() << ' ' << entry.level << ' ' << entry.message << '\n';
    stream_.flush();
}

}  // namespace logging
