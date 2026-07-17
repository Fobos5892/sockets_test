#include "crypto/EncryptedKeyStoreFile.hpp"
#include "crypto/MonocypherMessageCipher.hpp"
#include "crypto/PeerKeyStore.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

TEST(CryptoTest, PeerEncryptDecryptRoundTrip) {
    crypto::MonocypherMessageCipher cipher;
    const auto alice = cipher.generate_keypair();
    const auto bob = cipher.generate_keypair();

    const std::string plain = "hello e2e";
    const auto sealed = cipher.encrypt_for_peer(alice.secret_key, bob.public_key, plain);
    EXPECT_NE(sealed, std::vector<uint8_t>(plain.begin(), plain.end()));

    const std::string opened = cipher.decrypt_from_peer(bob.secret_key, alice.public_key, sealed);
    EXPECT_EQ(opened, plain);
}

TEST(CryptoTest, RoomKeySealAndBroadcast) {
    crypto::MonocypherMessageCipher cipher;
    const auto alice = cipher.generate_keypair();
    const auto bob = cipher.generate_keypair();
    const auto room = cipher.generate_room_key();

    const auto sealed = cipher.seal_room_key(alice.secret_key, bob.public_key, room);
    const auto opened = cipher.open_room_key(bob.secret_key, alice.public_key, sealed);
    EXPECT_EQ(opened, room);

    const auto ct = cipher.encrypt_with_room_key(room, "all:hello");
    EXPECT_EQ(cipher.decrypt_with_room_key(room, ct), "all:hello");
}

TEST(CryptoTest, KeystoreFileRoundTrip) {
    crypto::MonocypherMessageCipher cipher;
    crypto::EncryptedKeyStoreFile file(cipher);

    crypto::PeerKeyStore store;
    auto pair = cipher.generate_keypair();
    store.set_identity(pair.secret_key, pair.public_key);
    store.set_room_key(cipher.generate_room_key());
    store.set_peer_public(42, cipher.generate_keypair().public_key);

    const std::string path = "test_keystore_roundtrip.keys";
    const std::string password = "test-password";
    file.save(path, password, store);

    crypto::PeerKeyStore loaded;
    ASSERT_TRUE(file.load(path, password, loaded));
    EXPECT_EQ(loaded.identity_public(), store.identity_public());
    EXPECT_EQ(loaded.identity_secret(), store.identity_secret());
    ASSERT_TRUE(loaded.has_room_key());
    EXPECT_EQ(*loaded.room_key(), *store.room_key());
    ASSERT_TRUE(loaded.has_peer(42));
    EXPECT_EQ(*loaded.peer_public(42), *store.peer_public(42));

    std::remove(path.c_str());
}

TEST(CryptoTest, KeystoreRejectsWrongPassword) {
    crypto::MonocypherMessageCipher cipher;
    crypto::EncryptedKeyStoreFile file(cipher);
    crypto::PeerKeyStore store;
    auto pair = cipher.generate_keypair();
    store.set_identity(std::move(pair.secret_key), std::move(pair.public_key));

    const std::string path = "test_keystore_badpass.keys";
    file.save(path, "correct", store);

    crypto::PeerKeyStore loaded;
    EXPECT_THROW(file.load(path, "wrong", loaded), crypto::CipherError);
    std::remove(path.c_str());
}
