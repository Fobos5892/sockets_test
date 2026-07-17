#include "viewmodel/ServerViewModel.hpp"

#include "Protocol.hpp"
#include "model/ClientRecord.hpp"
#include "viewmodel/AdminCommandParser.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

ServerViewModel::ServerViewModel(ServerConfig config, IServerView& view,
                                 std::unique_ptr<IListenSocket> listen_socket,
                                 std::unique_ptr<server_model::IClientRegistry> registry,
                                 std::unique_ptr<IMessageTransport> transport,
                                 std::unique_ptr<MessageDispatcher> dispatcher)
    : config_(std::move(config)),
      view_(view),
      listen_socket_(std::move(listen_socket)),
      registry_(std::move(registry)),
      transport_(std::move(transport)),
      dispatcher_(std::move(dispatcher)),
      presence_(*registry_, *transport_),
      context_{view_, *registry_, *transport_, presence_, &banned_ids_} {}

void ServerViewModel::request_stop() {
    running_ = false;
    listen_socket_->close();
    // Wake client receive loops immediately so they can exit.
    for (const int fd : registry_->all_fds()) {
        ::shutdown(fd, SHUT_RDWR);
    }
}

bool ServerViewModel::is_listening() const {
    return listen_socket_->is_open();
}

uint16_t ServerViewModel::port() const {
    return listen_socket_->port();
}

void ServerViewModel::submit_admin_line(std::string line) {
    std::lock_guard<std::mutex> lock(admin_mutex_);
    admin_queue_.push_back(std::move(line));
}

bool ServerViewModel::is_banned(uint32_t id) const {
    return banned_ids_.find(id) != banned_ids_.end();
}

void ServerViewModel::broadcast_banned_ids(const std::vector<uint32_t>& ids) {
    const auto payload = protocol::encode_banned_ids(ids);
    for (const int fd : registry_->all_fds()) {
        const auto* client = registry_->find_by_fd(fd);
        if (!client || is_banned(client->id)) {
            continue;
        }
        transport_->send(fd, protocol::MsgType::BannedIds, payload);
    }
}

void ServerViewModel::ban_ids(const std::vector<uint32_t>& ids) {
    std::vector<uint32_t> newly_banned;
    for (const uint32_t id : ids) {
        if (id == 0) {
            continue;
        }
        if (banned_ids_.insert(id).second) {
            newly_banned.push_back(id);
        }
        free_ids_.erase(id);
    }
    if (newly_banned.empty()) {
        view_.show_admin_message("no new ids banned");
        return;
    }

    broadcast_banned_ids(newly_banned);

    for (const uint32_t id : newly_banned) {
        const auto fd = registry_->fd_by_id(id);
        if (!fd) {
            continue;
        }
        ::shutdown(*fd, SHUT_RDWR);
    }

    std::string msg = "banned ids=";
    for (size_t i = 0; i < newly_banned.size(); ++i) {
        if (i > 0) {
            msg += ',';
        }
        msg += std::to_string(newly_banned[i]);
    }
    view_.show_admin_message(msg);
}

void ServerViewModel::process_admin_queue() {
    std::vector<std::string> lines;
    {
        std::lock_guard<std::mutex> lock(admin_mutex_);
        lines.swap(admin_queue_);
    }
    for (const auto& line : lines) {
        const auto command = server_admin::parse_admin_command(line);
        if (!command) {
            view_.show_admin_message(server_admin::admin_help_text());
            continue;
        }
        ban_ids(command->ids);
    }
}

void ServerViewModel::run() {
    listen_socket_->open();
    view_.show_listening(config_.bind_ip, listen_socket_->port());

    std::vector<pollfd> pollfds;
    pollfds.push_back({listen_socket_->fd(), POLLIN, 0});

    while (running_) {
        process_admin_queue();

        const int ready = poll(pollfds.data(), pollfds.size(), kPollTimeoutMs);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (!running_) {
                break;
            }
            throw std::runtime_error("poll failed");
        }
        if (ready == 0) {
            continue;
        }

        for (size_t i = 1; i < pollfds.size(); ++i) {
            if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                if (running_) {
                    remove_client(pollfds[i].fd);
                }
                pollfds[i].fd = -1;
            }
        }

        if (!running_) {
            break;
        }

        for (size_t i = 0; i < pollfds.size(); ++i) {
            if (pollfds[i].fd < 0 || !(pollfds[i].revents & POLLIN)) {
                continue;
            }

            if (pollfds[i].fd == listen_socket_->fd()) {
                accept_client(pollfds);
                continue;
            }

            const int fd = pollfds[i].fd;
            handle_client_data(fd);
            if (!registry_->find_by_fd(fd)) {
                pollfds[i].fd = -1;
            }
        }

        pollfds.erase(
            std::remove_if(pollfds.begin(), pollfds.end(),
                           [](const pollfd& p) { return p.fd < 0; }),
            pollfds.end());
    }

    disconnect_all_clients();
}

void ServerViewModel::disconnect_all_clients() {
    const auto fds = registry_->all_fds();
    for (const int fd : fds) {
        const auto record = registry_->remove(fd);
        if (!record) {
            continue;
        }
        view_.show_client_disconnected(record->id, record->nickname);
        release_client_id(record->id);
        ::shutdown(fd, SHUT_RDWR);
        close(fd);
    }
}

uint32_t ServerViewModel::allocate_client_id() {
    while (!free_ids_.empty()) {
        const auto it = free_ids_.begin();
        const uint32_t id = *it;
        free_ids_.erase(it);
        if (!is_banned(id)) {
            return id;
        }
    }
    while (is_banned(next_id_)) {
        ++next_id_;
    }
    return next_id_++;
}

void ServerViewModel::release_client_id(uint32_t id) {
    if (is_banned(id)) {
        return;
    }
    free_ids_.insert(id);
}

void ServerViewModel::accept_client(std::vector<pollfd>& pollfds) {
    const int client_fd = listen_socket_->accept();
    if (client_fd < 0) {
        view_.show_accept_failed(std::strerror(errno));
        return;
    }

    const bool create_room = registry_->all_fds().empty();
    const auto record = registry_->add(client_fd, allocate_client_id());
    pollfds.push_back({client_fd, POLLIN, 0});

    view_.show_client_connected(record.id);
    transport_->send(client_fd, protocol::MsgType::AssignId,
                     protocol::encode_assign_id(record.id, create_room));
    presence_.user_joined(record.id, "", client_fd);
}

void ServerViewModel::remove_client(int fd) {
    const auto record = registry_->remove(fd);
    if (!record) {
        return;
    }

    view_.show_client_disconnected(record->id, record->nickname);
    release_client_id(record->id);
    close(fd);
    presence_.user_left(record->id, record->nickname);
}

void ServerViewModel::handle_client_data(int fd) {
    try {
        const auto* sender = registry_->find_by_fd(fd);
        if (sender && is_banned(sender->id)) {
            remove_client(fd);
            return;
        }

        const auto message = transport_->receive(fd);
        if (!message) {
            if (running_) {
                remove_client(fd);
            }
            return;
        }
        dispatcher_->dispatch(fd, *message, context_);
    } catch (const std::exception& ex) {
        view_.show_decode_error(ex.what());
        transport_->send(fd, protocol::MsgType::Error, protocol::encode_error(ex.what()));
    }
}
