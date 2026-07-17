#pragma once

#include "crypto/IMessageCipher.hpp"

namespace crypto {

class MonocypherMessageCipher final : public IMessageCipher {
public:
    KeyPair generate_keypair() override;
    std::vector<uint8_t> generate_room_key() override;

    std::vector<uint8_t> encrypt_for_peer(const std::vector<uint8_t>& my_secret,
                                          const std::vector<uint8_t>& peer_public,
                                          const std::string& plaintext) override;

    std::string decrypt_from_peer(const std::vector<uint8_t>& my_secret,
                                  const std::vector<uint8_t>& peer_public,
                                  const std::vector<uint8_t>& ciphertext) override;

    std::vector<uint8_t> seal_room_key(const std::vector<uint8_t>& my_secret,
                                       const std::vector<uint8_t>& peer_public,
                                       const std::vector<uint8_t>& room_key) override;

    std::vector<uint8_t> open_room_key(const std::vector<uint8_t>& my_secret,
                                       const std::vector<uint8_t>& peer_public,
                                       const std::vector<uint8_t>& sealed) override;

    std::vector<uint8_t> encrypt_with_room_key(const std::vector<uint8_t>& room_key,
                                               const std::string& plaintext) override;

    std::string decrypt_with_room_key(const std::vector<uint8_t>& room_key,
                                      const std::vector<uint8_t>& ciphertext) override;

    std::vector<uint8_t> encrypt_keystore(const std::string& password,
                                          const std::vector<uint8_t>& plaintext) override;

    std::vector<uint8_t> decrypt_keystore(const std::string& password,
                                          const std::vector<uint8_t>& ciphertext) override;
};

}  // namespace crypto
