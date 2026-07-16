#include "viewmodel/PresenceBroadcaster.hpp"

#include "Protocol.hpp"

PresenceBroadcaster::PresenceBroadcaster(server_model::IClientRegistry& registry, IMessageTransport& transport)
    : registry_(registry), transport_(transport) {}

void PresenceBroadcaster::user_joined(uint32_t id, const std::string& nickname, int except_fd) {
    broadcast(protocol::MsgType::UserJoined, protocol::encode_user_joined(id, nickname), except_fd);
}

void PresenceBroadcaster::user_left(uint32_t id, const std::string& nickname) {
    broadcast(protocol::MsgType::UserLeft, protocol::encode_user_left(id, nickname), -1);
}

void PresenceBroadcaster::broadcast(protocol::MsgType type, const std::vector<uint8_t>& payload, int except_fd) {
    for (const int client_fd : registry_.all_fds()) {
        if (client_fd != except_fd) {
            transport_.send(client_fd, type, payload);
        }
    }
}
