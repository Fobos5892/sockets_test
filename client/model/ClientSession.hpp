#pragma once

#include <cstdint>
#include <string>

#include "Config.hpp"

namespace client_model {

class ClientSession {
public:
    explicit ClientSession(ClientConfig config);

    const ClientConfig& config() const;
    int socket_fd() const;
    void set_socket_fd(int fd);
    uint32_t my_id() const;
    void set_my_id(uint32_t id);
    bool has_nickname() const;

private:
    ClientConfig config_;
    int socket_fd_{-1};
    uint32_t my_id_{0};
};

}  // namespace client_model
