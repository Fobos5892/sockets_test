#include "Server.hpp"

#include "model/ClientRegistry.hpp"
#include "transport/ModbusMessageTransport.hpp"
#include "transport/TcpListenSocket.hpp"
#include "viewmodel/MessageDispatcherFactory.hpp"

Server::Server(ServerConfig config)
    : view_model_(config, view_, std::make_unique<TcpListenSocket>(config),
                  std::make_unique<server_model::ClientRegistry>(),
                  std::make_unique<ModbusMessageTransport>(), create_message_dispatcher()) {}

void Server::run() {
    view_model_.run();
}

void Server::request_stop() {
    view_model_.request_stop();
}

bool Server::is_listening() const {
    return view_model_.is_listening();
}

uint16_t Server::port() const {
    return view_model_.port();
}
