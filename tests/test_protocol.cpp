#include "Protocol.hpp"

#include <gtest/gtest.h>

namespace {

void expect_roundtrip(const protocol::AppMessage& original) {
    std::vector<uint8_t> payload;
    switch (original.type) {
        case protocol::MsgType::Register:
            payload = protocol::encode_register(std::get<protocol::RegisterPayload>(original.payload).nickname);
            break;
        case protocol::MsgType::AssignId:
            payload = protocol::encode_assign_id(std::get<protocol::AssignIdPayload>(original.payload).id);
            break;
        case protocol::MsgType::Chat:
            payload = protocol::encode_chat(std::get<protocol::ChatPayload>(original.payload));
            break;
        case protocol::MsgType::Deliver:
            payload = protocol::encode_deliver(std::get<protocol::DeliverPayload>(original.payload).from_id,
                                               std::get<protocol::DeliverPayload>(original.payload).text);
            break;
        case protocol::MsgType::Error:
            payload = protocol::encode_error(std::get<protocol::ErrorPayload>(original.payload).text);
            break;
        case protocol::MsgType::UserJoined: {
            const auto& joined = std::get<protocol::UserJoinedPayload>(original.payload);
            payload = protocol::encode_user_joined(joined.id, joined.nickname);
            break;
        }
        case protocol::MsgType::UserLeft: {
            const auto& left = std::get<protocol::UserLeftPayload>(original.payload);
            payload = protocol::encode_user_left(left.id, left.nickname);
            break;
        }
        case protocol::MsgType::ListUsers:
            payload = protocol::encode_list_users();
            break;
        case protocol::MsgType::UsersList:
            payload = protocol::encode_users_list(std::get<protocol::UsersListPayload>(original.payload).users);
            break;
    }

    const modbus::Frame frame = protocol::make_frame(original.type, payload, 99);
    const protocol::AppMessage decoded = protocol::decode_app_message(frame);
    EXPECT_EQ(decoded.type, original.type);

    switch (original.type) {
        case protocol::MsgType::Register: {
            const auto& expected = std::get<protocol::RegisterPayload>(original.payload);
            const auto& actual = std::get<protocol::RegisterPayload>(decoded.payload);
            EXPECT_EQ(actual.nickname, expected.nickname);
            break;
        }
        case protocol::MsgType::AssignId: {
            const auto& expected = std::get<protocol::AssignIdPayload>(original.payload);
            const auto& actual = std::get<protocol::AssignIdPayload>(decoded.payload);
            EXPECT_EQ(actual.id, expected.id);
            break;
        }
        case protocol::MsgType::Chat: {
            const auto& expected = std::get<protocol::ChatPayload>(original.payload);
            const auto& actual = std::get<protocol::ChatPayload>(decoded.payload);
            EXPECT_EQ(actual.from_id, expected.from_id);
            EXPECT_EQ(actual.recipient_tag, expected.recipient_tag);
            EXPECT_EQ(actual.recipient_data, expected.recipient_data);
            EXPECT_EQ(actual.text, expected.text);
            break;
        }
        case protocol::MsgType::Deliver: {
            const auto& expected = std::get<protocol::DeliverPayload>(original.payload);
            const auto& actual = std::get<protocol::DeliverPayload>(decoded.payload);
            EXPECT_EQ(actual.from_id, expected.from_id);
            EXPECT_EQ(actual.text, expected.text);
            break;
        }
        case protocol::MsgType::Error: {
            const auto& expected = std::get<protocol::ErrorPayload>(original.payload);
            const auto& actual = std::get<protocol::ErrorPayload>(decoded.payload);
            EXPECT_EQ(actual.text, expected.text);
            break;
        }
        case protocol::MsgType::UserJoined: {
            const auto& expected = std::get<protocol::UserJoinedPayload>(original.payload);
            const auto& actual = std::get<protocol::UserJoinedPayload>(decoded.payload);
            EXPECT_EQ(actual.id, expected.id);
            EXPECT_EQ(actual.nickname, expected.nickname);
            break;
        }
        case protocol::MsgType::UserLeft: {
            const auto& expected = std::get<protocol::UserLeftPayload>(original.payload);
            const auto& actual = std::get<protocol::UserLeftPayload>(decoded.payload);
            EXPECT_EQ(actual.id, expected.id);
            EXPECT_EQ(actual.nickname, expected.nickname);
            break;
        }
        case protocol::MsgType::ListUsers:
            EXPECT_TRUE(std::holds_alternative<protocol::ListUsersPayload>(decoded.payload));
            break;
        case protocol::MsgType::UsersList: {
            const auto& expected = std::get<protocol::UsersListPayload>(original.payload);
            const auto& actual = std::get<protocol::UsersListPayload>(decoded.payload);
            ASSERT_EQ(actual.users.size(), expected.users.size());
            for (size_t i = 0; i < expected.users.size(); ++i) {
                EXPECT_EQ(actual.users[i].id, expected.users[i].id);
                EXPECT_EQ(actual.users[i].nickname, expected.users[i].nickname);
            }
            break;
        }
    }
}

}  // namespace

TEST(ProtocolTest, RegisterRoundTrip) {
    protocol::AppMessage message;
    message.type = protocol::MsgType::Register;
    message.payload = protocol::RegisterPayload{"alice"};
    expect_roundtrip(message);
}

TEST(ProtocolTest, AssignIdRoundTrip) {
    protocol::AppMessage message;
    message.type = protocol::MsgType::AssignId;
    message.payload = protocol::AssignIdPayload{3};
    expect_roundtrip(message);
}

TEST(ProtocolTest, ChatByIdRoundTrip) {
    protocol::AppMessage message;
    message.type = protocol::MsgType::Chat;
    message.payload = protocol::ChatPayload{
        1, 0x01, {0, 0, 0, 2}, "hello",
    };
    expect_roundtrip(message);
}

TEST(ProtocolTest, ChatByNicknameRoundTrip) {
    protocol::AppMessage message;
    message.type = protocol::MsgType::Chat;
    message.payload = protocol::ChatPayload{
        1, 0x02, protocol::encode_string("bob"), "hello",
    };
    expect_roundtrip(message);
}

TEST(ProtocolTest, DeliverRoundTrip) {
    protocol::AppMessage message;
    message.type = protocol::MsgType::Deliver;
    message.payload = protocol::DeliverPayload{1, "payload"};
    expect_roundtrip(message);
}

TEST(ProtocolTest, ErrorRoundTrip) {
    protocol::AppMessage message;
    message.type = protocol::MsgType::Error;
    message.payload = protocol::ErrorPayload{"Unknown recipient"};
    expect_roundtrip(message);
}

TEST(ProtocolTest, UserJoinedRoundTrip) {
    protocol::AppMessage message;
    message.type = protocol::MsgType::UserJoined;
    message.payload = protocol::UserJoinedPayload{2, "alice"};
    expect_roundtrip(message);
}

TEST(ProtocolTest, UserLeftRoundTrip) {
    protocol::AppMessage message;
    message.type = protocol::MsgType::UserLeft;
    message.payload = protocol::UserLeftPayload{2, "alice"};
    expect_roundtrip(message);
}

TEST(ProtocolTest, ListUsersRoundTrip) {
    protocol::AppMessage message;
    message.type = protocol::MsgType::ListUsers;
    message.payload = protocol::ListUsersPayload{};
    expect_roundtrip(message);
}

TEST(ProtocolTest, UsersListRoundTrip) {
    protocol::AppMessage message;
    message.type = protocol::MsgType::UsersList;
    message.payload = protocol::UsersListPayload{
        {{1, "alice"}, {2, ""}},
    };
    expect_roundtrip(message);
}

TEST(ProtocolTest, RejectsUnexpectedFunctionCode) {
    modbus::Frame frame;
    frame.pdu = {0x03, static_cast<uint8_t>(protocol::MsgType::Register)};
    EXPECT_THROW(protocol::decode_app_message(frame), std::runtime_error);
}
