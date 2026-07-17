#include "crypto/MonocypherMessageCipher.hpp"

#include "crypto/third_party/monocypher.h"

#include <fstream>
#include <random>

namespace crypto {
namespace {

void require_size(const std::vector<uint8_t>& data, size_t expected, const char* label) {
    if (data.size() != expected) {
        throw CipherError(std::string(label) + " has invalid size");
    }
}

void fill_random(uint8_t* out, size_t len) {
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (urandom) {
        urandom.read(reinterpret_cast<char*>(out), static_cast<std::streamsize>(len));
        if (static_cast<size_t>(urandom.gcount()) == len) {
            return;
        }
    }
    std::random_device rd;
    for (size_t i = 0; i < len; ++i) {
        out[i] = static_cast<uint8_t>(rd());
    }
}

std::vector<uint8_t> shared_secret(const std::vector<uint8_t>& my_secret,
                                   const std::vector<uint8_t>& peer_public) {
    require_size(my_secret, kSecretKeySize, "secret key");
    require_size(peer_public, kPublicKeySize, "peer public key");
    std::vector<uint8_t> shared(kSecretKeySize);
    crypto_x25519(shared.data(), my_secret.data(), peer_public.data());
    return shared;
}

std::vector<uint8_t> aead_encrypt(const uint8_t key[32], const uint8_t* plain, size_t plain_len) {
    std::vector<uint8_t> out(kNonceSize + plain_len + kMacSize);
    fill_random(out.data(), kNonceSize);
    uint8_t* mac = out.data() + kNonceSize;
    uint8_t* ct = out.data() + kNonceSize + kMacSize;
    crypto_aead_lock(ct, mac, key, out.data(), nullptr, 0, plain, plain_len);
    return out;
}

std::vector<uint8_t> aead_decrypt(const uint8_t key[32], const std::vector<uint8_t>& blob) {
    if (blob.size() < kNonceSize + kMacSize) {
        throw CipherError("ciphertext too short");
    }
    const size_t plain_len = blob.size() - kNonceSize - kMacSize;
    std::vector<uint8_t> plain(plain_len);
    const uint8_t* nonce = blob.data();
    const uint8_t* mac = blob.data() + kNonceSize;
    const uint8_t* ct = blob.data() + kNonceSize + kMacSize;
    if (crypto_aead_unlock(plain.data(), mac, key, nonce, nullptr, 0, ct, plain_len) != 0) {
        throw CipherError("decryption failed");
    }
    return plain;
}

void derive_password_key(const std::string& password, const uint8_t salt[16], uint8_t out_key[32]) {
    // Lightweight KDF for local keystore (resume project): BLAKE2b(password || salt).
    std::vector<uint8_t> msg(password.begin(), password.end());
    msg.insert(msg.end(), salt, salt + 16);
    crypto_blake2b(out_key, 32, msg.data(), msg.size());
}

}  // namespace

KeyPair MonocypherMessageCipher::generate_keypair() {
    KeyPair pair;
    pair.secret_key.resize(kSecretKeySize);
    pair.public_key.resize(kPublicKeySize);
    fill_random(pair.secret_key.data(), kSecretKeySize);
    crypto_x25519_public_key(pair.public_key.data(), pair.secret_key.data());
    return pair;
}

std::vector<uint8_t> MonocypherMessageCipher::generate_room_key() {
    std::vector<uint8_t> key(kRoomKeySize);
    fill_random(key.data(), kRoomKeySize);
    return key;
}

std::vector<uint8_t> MonocypherMessageCipher::encrypt_for_peer(const std::vector<uint8_t>& my_secret,
                                                              const std::vector<uint8_t>& peer_public,
                                                              const std::string& plaintext) {
    const auto shared = shared_secret(my_secret, peer_public);
    return aead_encrypt(shared.data(), reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.size());
}

std::string MonocypherMessageCipher::decrypt_from_peer(const std::vector<uint8_t>& my_secret,
                                                      const std::vector<uint8_t>& peer_public,
                                                      const std::vector<uint8_t>& ciphertext) {
    const auto shared = shared_secret(my_secret, peer_public);
    const auto plain = aead_decrypt(shared.data(), ciphertext);
    return std::string(plain.begin(), plain.end());
}

std::vector<uint8_t> MonocypherMessageCipher::seal_room_key(const std::vector<uint8_t>& my_secret,
                                                           const std::vector<uint8_t>& peer_public,
                                                           const std::vector<uint8_t>& room_key) {
    require_size(room_key, kRoomKeySize, "room key");
    const auto shared = shared_secret(my_secret, peer_public);
    return aead_encrypt(shared.data(), room_key.data(), room_key.size());
}

std::vector<uint8_t> MonocypherMessageCipher::open_room_key(const std::vector<uint8_t>& my_secret,
                                                           const std::vector<uint8_t>& peer_public,
                                                           const std::vector<uint8_t>& sealed) {
    const auto shared = shared_secret(my_secret, peer_public);
    auto room = aead_decrypt(shared.data(), sealed);
    if (room.size() != kRoomKeySize) {
        throw CipherError("invalid room key payload");
    }
    return room;
}

std::vector<uint8_t> MonocypherMessageCipher::encrypt_with_room_key(const std::vector<uint8_t>& room_key,
                                                                   const std::string& plaintext) {
    require_size(room_key, kRoomKeySize, "room key");
    return aead_encrypt(room_key.data(), reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.size());
}

std::string MonocypherMessageCipher::decrypt_with_room_key(const std::vector<uint8_t>& room_key,
                                                          const std::vector<uint8_t>& ciphertext) {
    require_size(room_key, kRoomKeySize, "room key");
    const auto plain = aead_decrypt(room_key.data(), ciphertext);
    return std::string(plain.begin(), plain.end());
}

std::vector<uint8_t> MonocypherMessageCipher::encrypt_keystore(const std::string& password,
                                                              const std::vector<uint8_t>& plaintext) {
    uint8_t salt[16];
    fill_random(salt, sizeof(salt));
    uint8_t key[32];
    derive_password_key(password, salt, key);
    auto sealed = aead_encrypt(key, plaintext.data(), plaintext.size());
    std::vector<uint8_t> out;
    out.insert(out.end(), salt, salt + sizeof(salt));
    out.insert(out.end(), sealed.begin(), sealed.end());
    crypto_wipe(key, sizeof(key));
    return out;
}

std::vector<uint8_t> MonocypherMessageCipher::decrypt_keystore(const std::string& password,
                                                              const std::vector<uint8_t>& ciphertext) {
    if (ciphertext.size() < 16 + kNonceSize + kMacSize) {
        throw CipherError("keystore blob too short");
    }
    const uint8_t* salt = ciphertext.data();
    uint8_t key[32];
    derive_password_key(password, salt, key);
    std::vector<uint8_t> sealed(ciphertext.begin() + 16, ciphertext.end());
    auto plain = aead_decrypt(key, sealed);
    crypto_wipe(key, sizeof(key));
    return plain;
}

}  // namespace crypto
