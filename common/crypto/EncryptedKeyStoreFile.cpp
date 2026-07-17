#include "crypto/EncryptedKeyStoreFile.hpp"

#include <fstream>
#include <iterator>

namespace crypto {

EncryptedKeyStoreFile::EncryptedKeyStoreFile(IMessageCipher& cipher) : cipher_(cipher) {}

void EncryptedKeyStoreFile::save(const std::string& path, const std::string& password,
                                 const PeerKeyStore& store) {
    const auto plain = store.serialize();
    const auto sealed = cipher_.encrypt_keystore(password, plain);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw CipherError("failed to open keystore for writing: " + path);
    }
    out.write(reinterpret_cast<const char*>(sealed.data()), static_cast<std::streamsize>(sealed.size()));
    if (!out) {
        throw CipherError("failed to write keystore: " + path);
    }
}

bool EncryptedKeyStoreFile::load(const std::string& path, const std::string& password,
                                 PeerKeyStore& store) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    std::vector<uint8_t> sealed((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (sealed.empty()) {
        return false;
    }
    const auto plain = cipher_.decrypt_keystore(password, sealed);
    store.deserialize(plain);
    return true;
}

}  // namespace crypto
