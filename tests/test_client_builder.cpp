#include "viewmodel/OutgoingMessageBuilder.hpp"

#include <gtest/gtest.h>

#include "Protocol.hpp"

TEST(ClientBuilderTest, BuildChatByIdRoundTrip) {
    const auto payload = client_viewmodel::OutgoingMessageBuilder::build_chat_by_id(1, 2, "hello");
    const modbus::Frame frame = protocol::make_frame(protocol::MsgType::Chat, payload);
    const protocol::AppMessage message = protocol::decode_app_message(frame);

    ASSERT_EQ(message.type, protocol::MsgType::Chat);
    const auto& chat = std::get<protocol::ChatPayload>(message.payload);
    EXPECT_EQ(chat.from_id, 1U);
    EXPECT_EQ(chat.recipient_tag, protocol::kRecipientById);
    EXPECT_EQ(chat.text, "hello");
}

TEST(ClientBuilderTest, BuildChatByNicknameRoundTrip) {
    const auto payload = client_viewmodel::OutgoingMessageBuilder::build_chat_by_nickname(1, "bob", "hey");
    const modbus::Frame frame = protocol::make_frame(protocol::MsgType::Chat, payload);
    const protocol::AppMessage message = protocol::decode_app_message(frame);

    ASSERT_EQ(message.type, protocol::MsgType::Chat);
    const auto& chat = std::get<protocol::ChatPayload>(message.payload);
    EXPECT_EQ(chat.from_id, 1U);
    EXPECT_EQ(chat.recipient_tag, protocol::kRecipientByNickname);
    EXPECT_EQ(protocol::decode_string(chat.recipient_data.data(), chat.recipient_data.size()), "bob");
    EXPECT_EQ(chat.text, "hey");
}

TEST(ClientBuilderTest, BuildChatBroadcastRoundTrip) {
    const auto payload = client_viewmodel::OutgoingMessageBuilder::build_chat_broadcast(1, "hi all");
    const modbus::Frame frame = protocol::make_frame(protocol::MsgType::Chat, payload);
    const protocol::AppMessage message = protocol::decode_app_message(frame);

    ASSERT_EQ(message.type, protocol::MsgType::Chat);
    const auto& chat = std::get<protocol::ChatPayload>(message.payload);
    EXPECT_EQ(chat.from_id, 1U);
    EXPECT_EQ(chat.recipient_tag, protocol::kRecipientBroadcast);
    EXPECT_TRUE(chat.recipient_data.empty());
    EXPECT_EQ(chat.text, "hi all");
}

TEST(ClientBuilderTest, BuildListUsersRoundTrip) {
    const auto payload = client_viewmodel::OutgoingMessageBuilder::build_list_users();
    const modbus::Frame frame = protocol::make_frame(protocol::MsgType::ListUsers, payload);
    const protocol::AppMessage message = protocol::decode_app_message(frame);
    EXPECT_EQ(message.type, protocol::MsgType::ListUsers);
}
