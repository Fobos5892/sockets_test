#include "viewmodel/ServerViewModel.hpp"

#include "model/ClientRecord.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <stdexcept>
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
      context_{view_, *registry_, *transport_, presence_} {}

void ServerViewModel::request_stop() {
    running_ = false;
    listen_socket_->close();
}

bool ServerViewModel::is_listening() const {
    return listen_socket_->is_open();
}

uint16_t ServerViewModel::port() const {
    return listen_socket_->port();
}

void ServerViewModel::run() {
    listen_socket_->open();
    view_.show_listening(config_.bind_ip, listen_socket_->port());

    std::vector<pollfd> pollfds;
    pollfds.push_back({listen_socket_->fd(), POLLIN, 0});

    while (running_) {
        const int ready = poll(pollfds.data(), pollfds.size(), 500);
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
                const int fd = pollfds[i].fd;
                remove_client(fd);
                pollfds[i].fd = -1;
            }
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
}

void ServerViewModel::accept_client(std::vector<pollfd>& pollfds) {
    const int client_fd = listen_socket_->accept();
    if (client_fd < 0) {
        view_.show_accept_failed(std::strerror(errno));
        return;
    }

    const auto record = registry_->add(client_fd, next_id_++);
    pollfds.push_back({client_fd, POLLIN, 0});

    view_.show_client_connected(record.id);
    transport_->send(client_fd, protocol::MsgType::AssignId, protocol::encode_assign_id(record.id));
    presence_.user_joined(record.id, "", client_fd);
}

void ServerViewModel::remove_client(int fd) {
    const auto record = registry_->remove(fd);
    if (!record) {
        return;
    }

    view_.show_client_disconnected(record->id, record->nickname);
    close(fd);
    presence_.user_left(record->id, record->nickname);
}

void ServerViewModel::handle_client_data(int fd) {
    try {
        const auto message = transport_->receive(fd);
        if (!message) {
            remove_client(fd);
            return;
        }
        dispatcher_->dispatch(fd, *message, context_);
    } catch (const std::exception& ex) {
        view_.show_decode_error(ex.what());
        transport_->send(fd, protocol::MsgType::Error, protocol::encode_error(ex.what()));
    }
}
