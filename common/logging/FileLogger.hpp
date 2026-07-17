#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

namespace logging {

class FileLogger {
public:
    FileLogger() = default;
    explicit FileLogger(std::string path);
    ~FileLogger();

    FileLogger(const FileLogger&) = delete;
    FileLogger& operator=(const FileLogger&) = delete;

    void open(std::string path);
    bool is_open() const;
    const std::string& path() const { return path_; }

    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    // Blocks until the writer queue is empty (entries already flushed to disk).
    void flush();

    // Stop accepting tasks, drain queue, join writer thread.
    void stop();

private:
    struct Entry {
        std::string level;
        std::string message;
    };

    void enqueue(const char* level, std::string message);
    void writer_loop();
    void write_entry(const Entry& entry);

    std::string path_;
    std::ofstream stream_;

    mutable std::mutex mutex_;
    std::condition_variable cv_has_work_;
    std::condition_variable cv_drained_;
    std::deque<Entry> queue_;
    bool running_{false};
    bool open_{false};
    std::thread writer_;
};

}  // namespace logging
