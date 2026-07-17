#pragma once

#include <string>

#include "crypto/IMessageCipher.hpp"
#include "crypto/PeerKeyStore.hpp"

namespace crypto {

class EncryptedKeyStoreFile {
public:
    explicit EncryptedKeyStoreFile(IMessageCipher& cipher);

    void save(const std::string& path, const std::string& password, const PeerKeyStore& store);
    bool load(const std::string& path, const std::string& password, PeerKeyStore& store);

private:
    IMessageCipher& cipher_;
};

}  // namespace crypto
