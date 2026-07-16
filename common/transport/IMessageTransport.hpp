#pragma once

#include <optional>
#include <vector>

#include "Protocol.hpp"

class IMessageTransport {
public:
    virtual ~IMessageTransport() = default;

    virtual bool send(int fd, protocol::MsgType type, const std::vector<uint8_t>& payload) = 0;
    virtual std::optional<protocol::AppMessage> receive(int fd) = 0;
};
