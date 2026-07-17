#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include <atomic>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "ModbusTcp.hpp"
#include "Protocol.hpp"

namespace test_utils {

inline void write_temp_config(const std::string& content, std::string& out_path) {
    static std::atomic<int> counter{0};
    const auto path = std::filesystem::temp_directory_path() /
                      ("modbus_test_" + std::to_string(getpid()) + "_" +
                       std::to_string(counter.fetch_add(1)) + ".conf");
    std::ofstream file(path);
    ASSERT_TRUE(file.is_open()) << "Failed to create temp config: " << path;
    file << content;
    out_path = path.string();
}

inline void tcp_connect(const std::string& ip, uint16_t port, int& out_fd) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0) << "socket failed: " << std::strerror(errno);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    ASSERT_GT(inet_pton(AF_INET, ip.c_str(), &addr.sin_addr), 0) << "invalid ip: " << ip;

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        const int connect_errno = errno;
        close(fd);
        FAIL() << "connect failed: " << std::strerror(connect_errno);
    }

    out_fd = fd;
}

inline void close_fd(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

inline void read_app_message(int fd, protocol::AppMessage& out) {
    std::vector<uint8_t> buffer;
    modbus::Frame frame;
    ASSERT_TRUE(modbus::ModbusTcp::read_frame(fd, buffer, frame)) << "read_frame failed";
    out = protocol::decode_app_message(frame);
}

inline void write_app_message(int fd, protocol::MsgType type, const std::vector<uint8_t>& payload,
                              uint16_t transaction_id = 0) {
    const modbus::Frame frame = protocol::make_frame(type, payload, transaction_id);
    ASSERT_TRUE(modbus::ModbusTcp::write_frame(fd, frame)) << "write_frame failed";
}

inline protocol::ChatPayload make_chat_by_id(uint32_t from_id, uint32_t recipient_id, const std::string& text) {
    protocol::ChatPayload chat;
    chat.from_id = from_id;
    chat.recipient_tag = protocol::kRecipientById;
    chat.recipient_data = protocol::encode_u32_be(recipient_id);
    chat.text = text;
    return chat;
}

inline protocol::ChatPayload make_chat_by_nickname(uint32_t from_id, const std::string& nickname,
                                                   const std::string& text) {
    protocol::ChatPayload chat;
    chat.from_id = from_id;
    chat.recipient_tag = protocol::kRecipientByNickname;
    chat.recipient_data = protocol::encode_string(nickname);
    chat.text = text;
    return chat;
}

inline protocol::ChatPayload make_chat_broadcast(uint32_t from_id, const std::string& text) {
    protocol::ChatPayload chat;
    chat.from_id = from_id;
    chat.recipient_tag = protocol::kRecipientBroadcast;
    chat.text = text;
    return chat;
}

}  // namespace test_utils
