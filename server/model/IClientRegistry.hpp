#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Protocol.hpp"
#include "model/ClientRecord.hpp"

namespace server_model {

class IClientRegistry {
public:
    virtual ~IClientRegistry() = default;

    virtual ClientRecord add(int fd, uint32_t id) = 0;
    virtual std::optional<ClientRecord> remove(int fd) = 0;
    virtual ClientRecord* find_by_fd(int fd) = 0;
    virtual const ClientRecord* find_by_fd(int fd) const = 0;
    virtual std::optional<int> fd_by_id(uint32_t id) const = 0;
    virtual std::optional<uint32_t> id_by_nickname(const std::string& nickname) const = 0;
    virtual bool nickname_taken_by_other(const std::string& nickname, uint32_t id) const = 0;
    virtual void set_nickname(int fd, const std::string& nickname) = 0;
    virtual std::vector<protocol::UserInfo> collect_users() const = 0;
    virtual std::optional<uint32_t> resolve_recipient(const protocol::ChatPayload& chat) const = 0;
    virtual std::string recipient_label(const protocol::ChatPayload& chat) const = 0;
    virtual std::vector<int> all_fds() const = 0;
};

}  // namespace server_model
