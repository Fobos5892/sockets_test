#include "transport/TcpListenSocket.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

namespace {

constexpr int kListenBacklog = 16;
constexpr int kReuseAddrEnabled = 1;

}  // namespace

TcpListenSocket::TcpListenSocket(ServerConfig config) : config_(std::move(config)) {}

void TcpListenSocket::open() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        throw std::runtime_error("Failed to create listen socket");
    }

    int opt = kReuseAddrEnabled;
    if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("setsockopt failed");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config_.port);
    if (inet_pton(AF_INET, config_.bind_ip.c_str(), &addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid bind_ip: " + config_.bind_ip);
    }

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("bind failed: " + std::string(std::strerror(errno)));
    }

    if (listen(listen_fd_, kListenBacklog) < 0) {
        throw std::runtime_error("listen failed");
    }

    sockaddr_in bound_addr{};
    socklen_t bound_len = sizeof(bound_addr);
    if (getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&bound_addr), &bound_len) == 0) {
        config_.port = ntohs(bound_addr.sin_port);
    }
}

void TcpListenSocket::close() {
    if (listen_fd_ >= 0) {
        ::shutdown(listen_fd_, SHUT_RDWR);
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
}

int TcpListenSocket::fd() const {
    return listen_fd_;
}

int TcpListenSocket::accept() {
    return ::accept(listen_fd_, nullptr, nullptr);
}

uint16_t TcpListenSocket::port() const {
    return config_.port;
}

bool TcpListenSocket::is_open() const {
    return listen_fd_ >= 0;
}
