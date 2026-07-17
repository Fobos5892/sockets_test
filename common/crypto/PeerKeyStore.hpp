#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace crypto {

class PeerKeyStore {
public:
    void set_identity(std::vector<uint8_t> secret_key, std::vector<uint8_t> public_key);
    const std::vector<uint8_t>& identity_secret() const { return identity_secret_; }
    const std::vector<uint8_t>& identity_public() const { return identity_public_; }
    bool has_identity() const { return !identity_secret_.empty(); }

    void set_room_key(std::vector<uint8_t> room_key);
    void clear_room_key();
    const std::optional<std::vector<uint8_t>>& room_key() const { return room_key_; }
    bool has_room_key() const { return room_key_.has_value(); }

    void set_peer_public(uint32_t peer_id, std::vector<uint8_t> public_key);
    void erase_peer(uint32_t peer_id);
    void erase_peers(const std::vector<uint32_t>& peer_ids);
    std::optional<std::vector<uint8_t>> peer_public(uint32_t peer_id) const;
    bool has_peer(uint32_t peer_id) const;
    const std::unordered_map<uint32_t, std::vector<uint8_t>>& peers() const { return peers_; }

    std::vector<uint8_t> serialize() const;
    void deserialize(const std::vector<uint8_t>& blob);

private:
    std::vector<uint8_t> identity_secret_;
    std::vector<uint8_t> identity_public_;
    std::optional<std::vector<uint8_t>> room_key_;
    std::unordered_map<uint32_t, std::vector<uint8_t>> peers_;
};

}  // namespace crypto
