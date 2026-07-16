#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <poll.h>
#include <vector>

#include "Config.hpp"
#include "model/IClientRegistry.hpp"
#include "transport/IListenSocket.hpp"
#include "transport/IMessageTransport.hpp"
#include "view/IServerView.hpp"
#include "viewmodel/MessageDispatcher.hpp"
#include "viewmodel/PresenceBroadcaster.hpp"
#include "viewmodel/ServerContext.hpp"

class ServerViewModel {
public:
    ServerViewModel(ServerConfig config, IServerView& view, std::unique_ptr<IListenSocket> listen_socket,
                    std::unique_ptr<server_model::IClientRegistry> registry,
                    std::unique_ptr<IMessageTransport> transport, std::unique_ptr<MessageDispatcher> dispatcher);

    void run();
    void request_stop();
    bool is_listening() const;
    uint16_t port() const;

private:
    void accept_client(std::vector<pollfd>& pollfds);
    void remove_client(int fd);
    void handle_client_data(int fd);

    ServerConfig config_;
    IServerView& view_;
    std::unique_ptr<IListenSocket> listen_socket_;
    std::unique_ptr<server_model::IClientRegistry> registry_;
    std::unique_ptr<IMessageTransport> transport_;
    std::unique_ptr<MessageDispatcher> dispatcher_;
    PresenceBroadcaster presence_;
    ServerContext context_;
    std::atomic<bool> running_{true};
    uint32_t next_id_{1};
};
