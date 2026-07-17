#include "model/ClientRegistry.hpp"

#include <algorithm>

namespace server_model {

ClientRecord ClientRegistry::add(int fd, uint32_t id) {
    ClientRecord record;
    record.fd = fd;
    record.id = id;
    clients_by_fd_[fd] = record;
    fd_by_id_[id] = fd;
    return record;
}

std::optional<ClientRecord> ClientRegistry::remove(int fd) {
    const auto it = clients_by_fd_.find(fd);
    if (it == clients_by_fd_.end()) {
        return std::nullopt;
    }

    ClientRecord record = it->second;
    if (!record.nickname.empty()) {
        id_by_nickname_.erase(record.nickname);
    }
    fd_by_id_.erase(record.id);
    clients_by_fd_.erase(it);
    return record;
}

ClientRecord* ClientRegistry::find_by_fd(int fd) {
    const auto it = clients_by_fd_.find(fd);
    return it == clients_by_fd_.end() ? nullptr : &it->second;
}

const ClientRecord* ClientRegistry::find_by_fd(int fd) const {
    const auto it = clients_by_fd_.find(fd);
    return it == clients_by_fd_.end() ? nullptr : &it->second;
}

std::optional<int> ClientRegistry::fd_by_id(uint32_t id) const {
    const auto it = fd_by_id_.find(id);
    if (it == fd_by_id_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<uint32_t> ClientRegistry::id_by_nickname(const std::string& nickname) const {
    const auto it = id_by_nickname_.find(nickname);
    if (it == id_by_nickname_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool ClientRegistry::nickname_taken_by_other(const std::string& nickname, uint32_t id) const {
    const auto it = id_by_nickname_.find(nickname);
    return it != id_by_nickname_.end() && it->second != id;
}

void ClientRegistry::set_nickname(int fd, const std::string& nickname) {
    auto* record = find_by_fd(fd);
    if (!record) {
        return;
    }
    if (!record->nickname.empty()) {
        id_by_nickname_.erase(record->nickname);
    }
    record->nickname = nickname;
    record->registered = true;
    id_by_nickname_[nickname] = record->id;
}

std::vector<protocol::UserInfo> ClientRegistry::collect_users() const {
    std::vector<protocol::UserInfo> users;
    users.reserve(clients_by_fd_.size());
    for (const auto& [fd, client] : clients_by_fd_) {
        (void)fd;
        users.push_back(protocol::UserInfo{client.id, client.nickname});
    }
    std::sort(users.begin(), users.end(),
              [](const protocol::UserInfo& a, const protocol::UserInfo& b) { return a.id < b.id; });
    return users;
}

std::optional<uint32_t> ClientRegistry::resolve_recipient(const protocol::ChatPayload& chat) const {
    if (chat.recipient_tag == protocol::kRecipientById) {
        if (chat.recipient_data.size() < protocol::kUint32Size) {
            return std::nullopt;
        }
        const uint32_t id = protocol::read_u32_be(chat.recipient_data.data());
        if (!fd_by_id(id)) {
            return std::nullopt;
        }
        return id;
    }

    if (chat.recipient_tag == protocol::kRecipientByNickname) {
        const std::string nickname =
            protocol::decode_string(chat.recipient_data.data(), chat.recipient_data.size());
        return id_by_nickname(nickname);
    }

    return std::nullopt;
}

std::string ClientRegistry::recipient_label(const protocol::ChatPayload& chat) const {
    if (chat.recipient_tag == protocol::kRecipientBroadcast) {
        return protocol::kBroadcastRecipient;
    }

    if (chat.recipient_tag == protocol::kRecipientById &&
        chat.recipient_data.size() >= protocol::kUint32Size) {
        return std::to_string(protocol::read_u32_be(chat.recipient_data.data()));
    }

    if (chat.recipient_tag == protocol::kRecipientByNickname) {
        return protocol::decode_string(chat.recipient_data.data(), chat.recipient_data.size());
    }

    return "?";
}

std::vector<int> ClientRegistry::all_fds() const {
    std::vector<int> fds;
    fds.reserve(clients_by_fd_.size());
    for (const auto& [fd, client] : clients_by_fd_) {
        (void)client;
        fds.push_back(fd);
    }
    return fds;
}

}  // namespace server_model
