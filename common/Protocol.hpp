#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "ModbusTcp.hpp"

namespace protocol {

enum class MsgType : uint8_t {
    Register = 0x01,
    AssignId = 0x02,
    Chat = 0x03,
    Deliver = 0x04,
    Error = 0x05,
    UserJoined = 0x06,
    UserLeft = 0x07,
    ListUsers = 0x08,
    UsersList = 0x09,
};

struct RegisterPayload {
    std::string nickname;
};

struct AssignIdPayload {
    uint32_t id{0};
};

struct ChatPayload {
    uint32_t from_id{0};
    uint8_t recipient_tag{0};
    std::vector<uint8_t> recipient_data;
    std::string text;
};

struct DeliverPayload {
    uint32_t from_id{0};
    std::string text;
};

struct ErrorPayload {
    std::string text;
};

struct UserInfo {
    uint32_t id{0};
    std::string nickname;
};

struct UserJoinedPayload {
    uint32_t id{0};
    std::string nickname;
};

struct UserLeftPayload {
    uint32_t id{0};
    std::string nickname;
};

struct ListUsersPayload {};

struct UsersListPayload {
    std::vector<UserInfo> users;
};

using AppPayload = std::variant<RegisterPayload, AssignIdPayload, ChatPayload, DeliverPayload, ErrorPayload,
                                UserJoinedPayload, UserLeftPayload, ListUsersPayload, UsersListPayload>;

struct AppMessage {
    MsgType type{};
    AppPayload payload;
};

std::vector<uint8_t> encode_string(const std::string& value);
std::string decode_string(const uint8_t* data, size_t len);

modbus::Frame make_frame(MsgType type, const std::vector<uint8_t>& payload, uint16_t transaction_id = 0);
AppMessage decode_app_message(const modbus::Frame& frame);

std::vector<uint8_t> encode_register(const std::string& nickname);
std::vector<uint8_t> encode_assign_id(uint32_t id);
std::vector<uint8_t> encode_chat(const ChatPayload& chat);
std::vector<uint8_t> encode_deliver(uint32_t from_id, const std::string& text);
std::vector<uint8_t> encode_error(const std::string& text);
std::vector<uint8_t> encode_user_joined(uint32_t id, const std::string& nickname);
std::vector<uint8_t> encode_user_left(uint32_t id, const std::string& nickname);
std::vector<uint8_t> encode_list_users();
std::vector<uint8_t> encode_users_list(const std::vector<UserInfo>& users);

}  // namespace protocol
