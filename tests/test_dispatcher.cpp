#include "viewmodel/MessageDispatcherFactory.hpp"

#include <gtest/gtest.h>

#include "model/ClientRegistry.hpp"
#include "testing/MemoryMessageTransport.hpp"
#include "testing/NullViews.hpp"
#include "testing/fixtures.hpp"
#include "viewmodel/PresenceBroadcaster.hpp"
#include "viewmodel/ServerContext.hpp"

namespace {

struct DispatcherFixture {
    NullServerView view;
    server_model::ClientRegistry registry;
    MemoryMessageTransport transport;
    PresenceBroadcaster presence{registry, transport};
    ServerContext context{view, registry, transport, presence};
    std::unique_ptr<MessageDispatcher> dispatcher{create_message_dispatcher()};
};

}  // namespace

TEST(DispatcherTest, RegisterSetsNicknameAndBroadcasts) {
    DispatcherFixture fx;
    fx.registry.add(1, 10);
    fx.registry.add(2, 20);

    fx.dispatcher->dispatch(1, testing_fixtures::make_register_message("alice"), fx.context);

    EXPECT_EQ(fx.registry.id_by_nickname("alice"), 10U);
    const auto joined = fx.transport.receive(2);
    ASSERT_TRUE(joined.has_value());
    EXPECT_EQ(joined->type, protocol::MsgType::UserJoined);
}

TEST(DispatcherTest, ChatDeliversToRecipient) {
    DispatcherFixture fx;
    fx.registry.add(1, 10);
    fx.registry.add(2, 20);
    fx.registry.set_nickname(1, "alice");
    fx.registry.set_nickname(2, "bob");

    const auto chat = testing_fixtures::make_chat_by_id(10, 20, "hello");
    fx.dispatcher->dispatch(1, testing_fixtures::make_chat_message(chat), fx.context);

    const auto delivered = fx.transport.receive(2);
    ASSERT_TRUE(delivered.has_value());
    ASSERT_EQ(delivered->type, protocol::MsgType::Deliver);
    const auto& payload = std::get<protocol::DeliverPayload>(delivered->payload);
    EXPECT_EQ(payload.from_id, 10U);
    EXPECT_EQ(payload.text, "hello");
}

TEST(DispatcherTest, ChatUnknownRecipientReturnsError) {
    DispatcherFixture fx;
    fx.registry.add(1, 10);

    const auto chat = testing_fixtures::make_chat_by_nickname(10, "ghost", "hi");
    fx.dispatcher->dispatch(1, testing_fixtures::make_chat_message(chat), fx.context);

    const auto error = fx.transport.receive(1);
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error->type, protocol::MsgType::Error);
}

TEST(DispatcherTest, ListUsersReturnsConnectedClients) {
    DispatcherFixture fx;
    fx.registry.add(1, 10);
    fx.registry.add(2, 20);
    fx.registry.set_nickname(1, "alice");
    fx.registry.set_nickname(2, "bob");

    fx.dispatcher->dispatch(1, testing_fixtures::make_list_users_message(), fx.context);

    const auto list = fx.transport.receive(1);
    ASSERT_TRUE(list.has_value());
    ASSERT_EQ(list->type, protocol::MsgType::UsersList);
    const auto& users = std::get<protocol::UsersListPayload>(list->payload).users;
    ASSERT_EQ(users.size(), 2U);
}
