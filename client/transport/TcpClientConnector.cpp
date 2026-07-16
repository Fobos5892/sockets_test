#include "transport/TcpClientConnector.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

int TcpClientConnector::connect(const ClientConfig& config) {
    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.port);
    if (inet_pton(AF_INET, config.server_ip.c_str(), &addr.sin_addr) <= 0) {
        close(socket_fd);
        throw std::runtime_error("Invalid server_ip: " + config.server_ip);
    }

    if (::connect(socket_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(socket_fd);
        throw std::runtime_error("connect failed: " + std::string(std::strerror(errno)));
    }

    return socket_fd;
}
