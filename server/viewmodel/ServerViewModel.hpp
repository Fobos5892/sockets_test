#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <poll.h>
#include <set>
#include <string>
#include <unordered_set>
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

    // Thread-safe: queue an admin console line (e.g. "ban:1,2").
    void submit_admin_line(std::string line);

    // Ban ids until server restart; disconnects active clients; notifies peers.
    void ban_ids(const std::vector<uint32_t>& ids);

    bool is_banned(uint32_t id) const;

private:
    void accept_client(std::vector<pollfd>& pollfds);
    void remove_client(int fd);
    void handle_client_data(int fd);
    void disconnect_all_clients();
    void process_admin_queue();
    void broadcast_banned_ids(const std::vector<uint32_t>& ids);
    uint32_t allocate_client_id();
    void release_client_id(uint32_t id);

    ServerConfig config_;
    IServerView& view_;
    std::unique_ptr<IListenSocket> listen_socket_;
    std::unique_ptr<server_model::IClientRegistry> registry_;
    std::unique_ptr<IMessageTransport> transport_;
    std::unique_ptr<MessageDispatcher> dispatcher_;
    PresenceBroadcaster presence_;
    ServerContext context_;
    std::atomic<bool> running_{true};
    static constexpr uint32_t kFirstClientId = 1;
    static constexpr int kPollTimeoutMs = 200;
    uint32_t next_id_{kFirstClientId};
    std::set<uint32_t> free_ids_;
    std::unordered_set<uint32_t> banned_ids_;

    mutable std::mutex admin_mutex_;
    std::vector<std::string> admin_queue_;
};
