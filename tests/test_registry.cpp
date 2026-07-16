#include "model/ClientRegistry.hpp"

#include <gtest/gtest.h>

#include "testing/fixtures.hpp"

TEST(RegistryTest, AddFindRemove) {
    server_model::ClientRegistry registry;
    registry.add(10, 1);
    registry.add(20, 2);

    ASSERT_NE(registry.find_by_fd(10), nullptr);
    EXPECT_EQ(registry.find_by_fd(10)->id, 1U);
    EXPECT_EQ(registry.fd_by_id(2), 20);

    const auto removed = registry.remove(10);
    ASSERT_TRUE(removed.has_value());
    EXPECT_EQ(removed->id, 1U);
    EXPECT_EQ(registry.find_by_fd(10), nullptr);
}

TEST(RegistryTest, NicknameLookupAndCollectUsers) {
    server_model::ClientRegistry registry;
    registry.add(10, 1);
    registry.add(20, 2);
    registry.set_nickname(10, "alice");
    registry.set_nickname(20, "bob");

    EXPECT_EQ(registry.id_by_nickname("bob"), 2U);
    EXPECT_TRUE(registry.nickname_taken_by_other("alice", 2));
    EXPECT_FALSE(registry.nickname_taken_by_other("alice", 1));

    const auto users = registry.collect_users();
    ASSERT_EQ(users.size(), 2U);
    EXPECT_EQ(users[0].id, 1U);
    EXPECT_EQ(users[1].id, 2U);
}

TEST(RegistryTest, ResolveRecipientByIdAndNickname) {
    server_model::ClientRegistry registry;
    registry.add(10, 1);
    registry.add(20, 2);
    registry.set_nickname(20, "bob");

    const auto by_id = testing_fixtures::make_chat_by_id(1, 2, "hi");
    EXPECT_EQ(registry.resolve_recipient(by_id), 2U);

    const auto by_nick = testing_fixtures::make_chat_by_nickname(1, "bob", "hi");
    EXPECT_EQ(registry.resolve_recipient(by_nick), 2U);

    const auto missing = testing_fixtures::make_chat_by_nickname(1, "carol", "hi");
    EXPECT_FALSE(registry.resolve_recipient(missing).has_value());
}
