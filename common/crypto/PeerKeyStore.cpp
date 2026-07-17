#include "crypto/PeerKeyStore.hpp"

#include "Protocol.hpp"
#include "crypto/IMessageCipher.hpp"

#include <stdexcept>

namespace crypto {
namespace {

constexpr uint8_t kKeyStoreVersion = 1;

}  // namespace

void PeerKeyStore::set_identity(std::vector<uint8_t> secret_key, std::vector<uint8_t> public_key) {
    if (secret_key.size() != kSecretKeySize || public_key.size() != kPublicKeySize) {
        throw std::runtime_error("invalid identity key sizes");
    }
    identity_secret_ = std::move(secret_key);
    identity_public_ = std::move(public_key);
}

void PeerKeyStore::set_room_key(std::vector<uint8_t> room_key) {
    if (room_key.size() != kRoomKeySize) {
        throw std::runtime_error("invalid room key size");
    }
    room_key_ = std::move(room_key);
}

void PeerKeyStore::clear_room_key() {
    room_key_.reset();
}

void PeerKeyStore::set_peer_public(uint32_t peer_id, std::vector<uint8_t> public_key) {
    if (public_key.size() != kPublicKeySize) {
        throw std::runtime_error("invalid peer public key size");
    }
    peers_[peer_id] = std::move(public_key);
}

void PeerKeyStore::erase_peer(uint32_t peer_id) {
    peers_.erase(peer_id);
}

void PeerKeyStore::erase_peers(const std::vector<uint32_t>& peer_ids) {
    for (const uint32_t id : peer_ids) {
        peers_.erase(id);
    }
}

std::optional<std::vector<uint8_t>> PeerKeyStore::peer_public(uint32_t peer_id) const {
    const auto it = peers_.find(peer_id);
    if (it == peers_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool PeerKeyStore::has_peer(uint32_t peer_id) const {
    return peers_.find(peer_id) != peers_.end();
}

std::vector<uint8_t> PeerKeyStore::serialize() const {
    if (!has_identity()) {
        throw std::runtime_error("cannot serialize keystore without identity");
    }
    std::vector<uint8_t> out;
    out.push_back(kKeyStoreVersion);
    out.insert(out.end(), identity_secret_.begin(), identity_secret_.end());
    out.insert(out.end(), identity_public_.begin(), identity_public_.end());
    out.push_back(room_key_ ? 1 : 0);
    if (room_key_) {
        out.insert(out.end(), room_key_->begin(), room_key_->end());
    }
    if (peers_.size() > protocol::kMaxU16Count) {
        throw std::runtime_error("too many peer keys");
    }
    protocol::write_u16_be(out, static_cast<uint16_t>(peers_.size()));
    for (const auto& [id, pk] : peers_) {
        protocol::write_u32_be(out, id);
        out.insert(out.end(), pk.begin(), pk.end());
    }
    return out;
}

void PeerKeyStore::deserialize(const std::vector<uint8_t>& blob) {
    size_t offset = 0;
    if (blob.size() < 1 + kSecretKeySize + kPublicKeySize + 1 + protocol::kUint16Size) {
        throw std::runtime_error("keystore blob too short");
    }
    if (blob[offset++] != kKeyStoreVersion) {
        throw std::runtime_error("unsupported keystore version");
    }

    identity_secret_.assign(blob.begin() + static_cast<std::ptrdiff_t>(offset),
                            blob.begin() + static_cast<std::ptrdiff_t>(offset + kSecretKeySize));
    offset += kSecretKeySize;
    identity_public_.assign(blob.begin() + static_cast<std::ptrdiff_t>(offset),
                            blob.begin() + static_cast<std::ptrdiff_t>(offset + kPublicKeySize));
    offset += kPublicKeySize;

    const bool has_room = blob[offset++] != 0;
    room_key_.reset();
    if (has_room) {
        if (blob.size() < offset + kRoomKeySize) {
            throw std::runtime_error("truncated room key in keystore");
        }
        room_key_ = std::vector<uint8_t>(blob.begin() + static_cast<std::ptrdiff_t>(offset),
                                         blob.begin() + static_cast<std::ptrdiff_t>(offset + kRoomKeySize));
        offset += kRoomKeySize;
    }

    if (blob.size() < offset + protocol::kUint16Size) {
        throw std::runtime_error("truncated peer count in keystore");
    }
    const uint16_t count = protocol::read_u16_be(blob.data() + offset);
    offset += protocol::kUint16Size;

    peers_.clear();
    for (uint16_t i = 0; i < count; ++i) {
        if (blob.size() < offset + protocol::kUint32Size + kPublicKeySize) {
            throw std::runtime_error("truncated peer entry in keystore");
        }
        const uint32_t id = protocol::read_u32_be(blob.data() + offset);
        offset += protocol::kUint32Size;
        std::vector<uint8_t> pk(blob.begin() + static_cast<std::ptrdiff_t>(offset),
                                blob.begin() + static_cast<std::ptrdiff_t>(offset + kPublicKeySize));
        offset += kPublicKeySize;
        peers_[id] = std::move(pk);
    }
}

}  // namespace crypto
