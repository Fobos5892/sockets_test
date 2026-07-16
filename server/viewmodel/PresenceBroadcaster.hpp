#pragma once

#include <string>
#include <vector>

#include "Protocol.hpp"
#include "model/IClientRegistry.hpp"
#include "transport/IMessageTransport.hpp"

class PresenceBroadcaster {
public:
    PresenceBroadcaster(server_model::IClientRegistry& registry, IMessageTransport& transport);

    void user_joined(uint32_t id, const std::string& nickname, int except_fd);
    void user_left(uint32_t id, const std::string& nickname);

private:
    void broadcast(protocol::MsgType type, const std::vector<uint8_t>& payload, int except_fd);

    server_model::IClientRegistry& registry_;
    IMessageTransport& transport_;
};
