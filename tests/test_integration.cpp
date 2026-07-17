#include "Server.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "tests/test_utils.hpp"

namespace {

class ServerFixture : public ::testing::Test {
protected:
    void SetUp() override {
        ServerConfig config;
        config.bind_ip = "127.0.0.1";
        config.port = 0;
        server_ = std::make_unique<Server>(config);
        server_thread_ = std::thread([this]() { server_->run(); });
        wait_for_server();
    }

    void TearDown() override {
        if (server_) {
            server_->request_stop();
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        server_.reset();
    }

    void wait_for_server() {
        for (int attempt = 0; attempt < 50; ++attempt) {
            if (server_->is_listening()) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        FAIL() << "Server did not start in time";
    }

    int connect_client() {
        int fd = -1;
        test_utils::tcp_connect("127.0.0.1", server_->port(), fd);
        return fd;
    }

    uint32_t read_assigned_id(int fd) {
        protocol::AppMessage message;
        test_utils::read_app_message(fd, message);
        EXPECT_EQ(message.type, protocol::MsgType::AssignId);
        return std::get<protocol::AssignIdPayload>(message.payload).id;
    }

    std::unique_ptr<Server> server_;
    std::thread server_thread_;
};

}  // namespace

TEST_F(ServerFixture, AssignsIncrementalIds) {
    const int client1 = connect_client();
    const int client2 = connect_client();

    EXPECT_EQ(read_assigned_id(client1), 1U);
    EXPECT_EQ(read_assigned_id(client2), 2U);

    test_utils::close_fd(client1);
    test_utils::close_fd(client2);
}

TEST_F(ServerFixture, DeliversChatByRecipientId) {
    const int sender_fd = connect_client();
    const int receiver_fd = connect_client();
    const uint32_t sender_id = read_assigned_id(sender_fd);
    const uint32_t receiver_id = read_assigned_id(receiver_fd);

    const auto chat = test_utils::make_chat_by_id(sender_id, receiver_id, "hello by id");
    test_utils::write_app_message(sender_fd, protocol::MsgType::Chat, protocol::encode_chat(chat));

    protocol::AppMessage delivered;
    test_utils::read_app_message(receiver_fd, delivered);
    ASSERT_EQ(delivered.type, protocol::MsgType::Deliver);
    const auto& payload = std::get<protocol::DeliverPayload>(delivered.payload);
    EXPECT_EQ(payload.from_id, sender_id);
    EXPECT_EQ(payload.text, "hello by id");

    test_utils::close_fd(sender_fd);
    test_utils::close_fd(receiver_fd);
}

TEST_F(ServerFixture, DeliversChatByNickname) {
    const int sender_fd = connect_client();
    const int receiver_fd = connect_client();
    const uint32_t sender_id = read_assigned_id(sender_fd);
    read_assigned_id(receiver_fd);

    test_utils::write_app_message(receiver_fd, protocol::MsgType::Register,
                                  protocol::encode_register("bob"));

    const auto chat = test_utils::make_chat_by_nickname(sender_id, "bob", "hello bob");
    test_utils::write_app_message(sender_fd, protocol::MsgType::Chat, protocol::encode_chat(chat));

    protocol::AppMessage delivered;
    test_utils::read_app_message(receiver_fd, delivered);
    ASSERT_EQ(delivered.type, protocol::MsgType::Deliver);
    const auto& payload = std::get<protocol::DeliverPayload>(delivered.payload);
    EXPECT_EQ(payload.from_id, sender_id);
    EXPECT_EQ(payload.text, "hello bob");

    test_utils::close_fd(sender_fd);
    test_utils::close_fd(receiver_fd);
}

TEST_F(ServerFixture, ReturnsErrorForUnknownRecipient) {
    const int sender_fd = connect_client();
    read_assigned_id(sender_fd);

    const auto chat = test_utils::make_chat_by_id(1, 99, "missing");
    test_utils::write_app_message(sender_fd, protocol::MsgType::Chat, protocol::encode_chat(chat));

    protocol::AppMessage error;
    test_utils::read_app_message(sender_fd, error);
    ASSERT_EQ(error.type, protocol::MsgType::Error);
    EXPECT_EQ(std::get<protocol::ErrorPayload>(error.payload).text, "Unknown recipient: 99");

    test_utils::close_fd(sender_fd);
}

TEST_F(ServerFixture, RejectsDuplicateNickname) {
    const int client1 = connect_client();
    const int client2 = connect_client();
    read_assigned_id(client1);
    read_assigned_id(client2);

    test_utils::write_app_message(client1, protocol::MsgType::Register, protocol::encode_register("alice"));

    protocol::AppMessage joined;
    test_utils::read_app_message(client2, joined);
    ASSERT_EQ(joined.type, protocol::MsgType::UserJoined);
    EXPECT_EQ(std::get<protocol::UserJoinedPayload>(joined.payload).nickname, "alice");

    test_utils::write_app_message(client2, protocol::MsgType::Register, protocol::encode_register("alice"));

    protocol::AppMessage error;
    test_utils::read_app_message(client2, error);
    ASSERT_EQ(error.type, protocol::MsgType::Error);
    EXPECT_EQ(std::get<protocol::ErrorPayload>(error.payload).text, "Nickname already taken");

    test_utils::close_fd(client1);
    test_utils::close_fd(client2);
}

TEST_F(ServerFixture, BroadcastsUserJoinedToExistingClients) {
    const int client1 = connect_client();
    const uint32_t client1_id = read_assigned_id(client1);

    const int client2 = connect_client();
    const uint32_t client2_id = read_assigned_id(client2);

    protocol::AppMessage joined;
    test_utils::read_app_message(client1, joined);
    ASSERT_EQ(joined.type, protocol::MsgType::UserJoined);
    const auto& payload = std::get<protocol::UserJoinedPayload>(joined.payload);
    EXPECT_EQ(payload.id, client2_id);
    EXPECT_TRUE(payload.nickname.empty());

    test_utils::close_fd(client1);
    test_utils::close_fd(client2);
    (void)client1_id;
}

TEST_F(ServerFixture, ListUsersReturnsConnectedClients) {
    const int client1 = connect_client();
    const uint32_t client1_id = read_assigned_id(client1);

    const int client2 = connect_client();
    const uint32_t client2_id = read_assigned_id(client2);
    protocol::AppMessage ignored;
    test_utils::read_app_message(client1, ignored);

    test_utils::write_app_message(client1, protocol::MsgType::Register, protocol::encode_register("alice"));
    test_utils::read_app_message(client2, ignored);

    test_utils::write_app_message(client1, protocol::MsgType::ListUsers, protocol::encode_list_users());

    protocol::AppMessage response;
    test_utils::read_app_message(client1, response);
    ASSERT_EQ(response.type, protocol::MsgType::UsersList);
    const auto& users = std::get<protocol::UsersListPayload>(response.payload).users;
    ASSERT_EQ(users.size(), 2U);
    EXPECT_EQ(users[0].id, client1_id);
    EXPECT_EQ(users[0].nickname, "alice");
    EXPECT_EQ(users[1].id, client2_id);
    EXPECT_TRUE(users[1].nickname.empty());

    test_utils::close_fd(client1);
    test_utils::close_fd(client2);
}

TEST_F(ServerFixture, BroadcastsUserLeftOnDisconnect) {
    const int client1 = connect_client();
    read_assigned_id(client1);

    const int client2 = connect_client();
    read_assigned_id(client2);
    protocol::AppMessage ignored;
    test_utils::read_app_message(client1, ignored);

    test_utils::close_fd(client2);

    protocol::AppMessage left;
    test_utils::read_app_message(client1, left);
    ASSERT_EQ(left.type, protocol::MsgType::UserLeft);
    EXPECT_EQ(std::get<protocol::UserLeftPayload>(left.payload).id, 2U);

    test_utils::close_fd(client1);
}
