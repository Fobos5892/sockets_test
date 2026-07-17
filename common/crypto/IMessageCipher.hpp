#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace crypto {

constexpr size_t kPublicKeySize = 32;
constexpr size_t kSecretKeySize = 32;
constexpr size_t kRoomKeySize = 32;
constexpr size_t kNonceSize = 24;  // XChaCha20-Poly1305 / Monocypher aead
constexpr size_t kMacSize = 16;

struct KeyPair {
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> secret_key;
};

class IMessageCipher {
public:
    virtual ~IMessageCipher() = default;

    virtual KeyPair generate_keypair() = 0;
    virtual std::vector<uint8_t> generate_room_key() = 0;

    // AEAD encrypt plaintext for peer (ECDH(my_secret, peer_public) -> session key).
    virtual std::vector<uint8_t> encrypt_for_peer(const std::vector<uint8_t>& my_secret,
                                                  const std::vector<uint8_t>& peer_public,
                                                  const std::string& plaintext) = 0;

    virtual std::string decrypt_from_peer(const std::vector<uint8_t>& my_secret,
                                          const std::vector<uint8_t>& peer_public,
                                          const std::vector<uint8_t>& ciphertext) = 0;

    // Seal room key to peer / open sealed room key from peer.
    virtual std::vector<uint8_t> seal_room_key(const std::vector<uint8_t>& my_secret,
                                               const std::vector<uint8_t>& peer_public,
                                               const std::vector<uint8_t>& room_key) = 0;

    virtual std::vector<uint8_t> open_room_key(const std::vector<uint8_t>& my_secret,
                                               const std::vector<uint8_t>& peer_public,
                                               const std::vector<uint8_t>& sealed) = 0;

    // Symmetric encrypt/decrypt with room key (for all:).
    virtual std::vector<uint8_t> encrypt_with_room_key(const std::vector<uint8_t>& room_key,
                                                       const std::string& plaintext) = 0;

    virtual std::string decrypt_with_room_key(const std::vector<uint8_t>& room_key,
                                              const std::vector<uint8_t>& ciphertext) = 0;

    // Encrypt/decrypt keystore blob with password-derived key.
    virtual std::vector<uint8_t> encrypt_keystore(const std::string& password,
                                                  const std::vector<uint8_t>& plaintext) = 0;

    virtual std::vector<uint8_t> decrypt_keystore(const std::string& password,
                                                  const std::vector<uint8_t>& ciphertext) = 0;
};

class CipherError : public std::runtime_error {
public:
    explicit CipherError(const std::string& what) : std::runtime_error(what) {}
};

}  // namespace crypto
