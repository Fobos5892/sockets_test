#pragma once

#include <deque>
#include <unordered_map>
#include <vector>

#include "ModbusTcp.hpp"
#include "Protocol.hpp"
#include "transport/IMessageTransport.hpp"

class MemoryMessageTransport final : public IMessageTransport {
public:
    bool send(int fd, protocol::MsgType type, const std::vector<uint8_t>& payload) override {
        const modbus::Frame frame = protocol::make_frame(type, payload);
        const std::vector<uint8_t> bytes = modbus::ModbusTcp::encode(frame);
        queues_[fd].push_back(bytes);
        return true;
    }

    std::optional<protocol::AppMessage> receive(int fd) override {
        auto it = queues_.find(fd);
        if (it == queues_.end() || it->second.empty()) {
            return std::nullopt;
        }

        const std::vector<uint8_t> bytes = std::move(it->second.front());
        it->second.pop_front();

        size_t consumed = 0;
        const auto frame = modbus::ModbusTcp::decode(bytes, consumed);
        if (!frame) {
            return std::nullopt;
        }
        return protocol::decode_app_message(*frame);
    }

    size_t pending(int fd) const {
        const auto it = queues_.find(fd);
        return it == queues_.end() ? 0 : it->second.size();
    }

    void clear() { queues_.clear(); }

private:
    std::unordered_map<int, std::deque<std::vector<uint8_t>>> queues_;
};
