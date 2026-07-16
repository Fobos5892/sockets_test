#include "model/ClientSession.hpp"

namespace client_model {

ClientSession::ClientSession(ClientConfig config) : config_(std::move(config)) {}

const ClientConfig& ClientSession::config() const {
    return config_;
}

int ClientSession::socket_fd() const {
    return socket_fd_;
}

void ClientSession::set_socket_fd(int fd) {
    socket_fd_ = fd;
}

uint32_t ClientSession::my_id() const {
    return my_id_;
}

void ClientSession::set_my_id(uint32_t id) {
    my_id_ = id;
}

bool ClientSession::has_nickname() const {
    return !config_.nickname.empty();
}

}  // namespace client_model
