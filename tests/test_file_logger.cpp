#include "logging/FileLogger.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

TEST(FileLoggerTest, WritesLevelsToFile) {
    const std::string path = "test_file_logger.log";
    std::remove(path.c_str());

    logging::FileLogger logger(path);
    ASSERT_TRUE(logger.is_open());
    logger.info("hello-info");
    logger.warn("hello-warn");
    logger.error("hello-error");
    logger.flush();

    std::ifstream in(path);
    ASSERT_TRUE(in.is_open());
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("INFO hello-info"), std::string::npos);
    EXPECT_NE(content.find("WARN hello-warn"), std::string::npos);
    EXPECT_NE(content.find("ERROR hello-error"), std::string::npos);

    std::remove(path.c_str());
}

TEST(FileLoggerTest, ConcurrentEnqueueDrainsInOrder) {
    const std::string path = "test_file_logger_concurrent.log";
    std::remove(path.c_str());

    logging::FileLogger logger(path);
    ASSERT_TRUE(logger.is_open());

    constexpr int kPerThread = 50;
    constexpr int kThreads = 4;
    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&logger, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                logger.info("t" + std::to_string(t) + "-" + std::to_string(i));
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }
    logger.flush();

    std::ifstream in(path);
    ASSERT_TRUE(in.is_open());
    int lines = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) {
            ++lines;
        }
    }
    EXPECT_EQ(lines, kPerThread * kThreads);

    std::remove(path.c_str());
}
