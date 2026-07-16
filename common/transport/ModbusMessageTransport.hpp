#pragma once

#include "transport/IMessageTransport.hpp"

class ModbusMessageTransport final : public IMessageTransport {
public:
    bool send(int fd, protocol::MsgType type, const std::vector<uint8_t>& payload) override;
    std::optional<protocol::AppMessage> receive(int fd) override;
};
