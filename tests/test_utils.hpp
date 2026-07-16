#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ModbusTcp.hpp"
#include "Protocol.hpp"

namespace test_utils {

inline std::string write_temp_config(const std::string& content) {
    static std::atomic<int> counter{0};
    const auto path = std::filesystem::temp_directory_path() /
                      ("modbus_test_" + std::to_string(getpid()) + "_" +
                       std::to_string(counter.fetch_add(1)) + ".conf");
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create temp config");
    }
    file << content;
    return path.string();
}

inline int tcp_connect(const std::string& ip, uint16_t port) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("socket failed");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        close(fd);
        throw std::runtime_error("invalid ip");
    }

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        throw std::runtime_error("connect failed: " + std::string(std::strerror(errno)));
    }

    return fd;
}

inline void close_fd(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

inline protocol::AppMessage read_app_message(int fd) {
    std::vector<uint8_t> buffer;
    modbus::Frame frame;
    if (!modbus::ModbusTcp::read_frame(fd, buffer, frame)) {
        throw std::runtime_error("read_frame failed");
    }
    return protocol::decode_app_message(frame);
}

inline void write_app_message(int fd, protocol::MsgType type, const std::vector<uint8_t>& payload,
                              uint16_t transaction_id = 0) {
    const modbus::Frame frame = protocol::make_frame(type, payload, transaction_id);
    if (!modbus::ModbusTcp::write_frame(fd, frame)) {
        throw std::runtime_error("write_frame failed");
    }
}

inline protocol::ChatPayload make_chat_by_id(uint32_t from_id, uint32_t recipient_id, const std::string& text) {
    protocol::ChatPayload chat;
    chat.from_id = from_id;
    chat.recipient_tag = 0x01;
    chat.recipient_data = {
        static_cast<uint8_t>((recipient_id >> 24) & 0xFF),
        static_cast<uint8_t>((recipient_id >> 16) & 0xFF),
        static_cast<uint8_t>((recipient_id >> 8) & 0xFF),
        static_cast<uint8_t>(recipient_id & 0xFF),
    };
    chat.text = text;
    return chat;
}

inline protocol::ChatPayload make_chat_by_nickname(uint32_t from_id, const std::string& nickname,
                                                   const std::string& text) {
    protocol::ChatPayload chat;
    chat.from_id = from_id;
    chat.recipient_tag = 0x02;
    chat.recipient_data = protocol::encode_string(nickname);
    chat.text = text;
    return chat;
}

}  // namespace test_utils
