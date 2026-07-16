#pragma once

#include "Config.hpp"
#include "transport/IListenSocket.hpp"

class TcpListenSocket final : public IListenSocket {
public:
    explicit TcpListenSocket(ServerConfig config);

    void open() override;
    void close() override;
    int fd() const override;
    int accept() override;
    uint16_t port() const override;
    bool is_open() const override;

private:
    ServerConfig config_;
    int listen_fd_{-1};
};
