#pragma once

#include <unordered_map>

#include "model/ClientRecord.hpp"
#include "model/IClientRegistry.hpp"

namespace server_model {

class ClientRegistry final : public IClientRegistry {
public:
    ClientRecord add(int fd, uint32_t id) override;
    std::optional<ClientRecord> remove(int fd) override;
    ClientRecord* find_by_fd(int fd) override;
    const ClientRecord* find_by_fd(int fd) const override;
    std::optional<int> fd_by_id(uint32_t id) const override;
    std::optional<uint32_t> id_by_nickname(const std::string& nickname) const override;
    bool nickname_taken_by_other(const std::string& nickname, uint32_t id) const override;
    void set_nickname(int fd, const std::string& nickname) override;
    std::vector<protocol::UserInfo> collect_users() const override;
    std::optional<uint32_t> resolve_recipient(const protocol::ChatPayload& chat) const override;
    std::string recipient_label(const protocol::ChatPayload& chat) const override;
    std::vector<int> all_fds() const override;

private:
    std::unordered_map<int, ClientRecord> clients_by_fd_;
    std::unordered_map<uint32_t, int> fd_by_id_;
    std::unordered_map<std::string, uint32_t> id_by_nickname_;
};

}  // namespace server_model
