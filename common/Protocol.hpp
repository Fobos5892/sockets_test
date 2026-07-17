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
    KeyOffer = 0x0A,      // client -> server (ChatPayload layout, body = pubkey)
    RoomKeyOffer = 0x0B,  // client -> server (ChatPayload layout, body = sealed room key)
    BannedIds = 0x0C,     // server -> clients
    PeerKey = 0x0D,       // server -> client (DeliverPayload layout, body = pubkey)
    RoomKey = 0x0E,       // server -> client (DeliverPayload layout, body = sealed room key)
};

constexpr size_t kUint16Size = 2;
constexpr size_t kUint32Size = 4;
constexpr size_t kPduAppHeaderSize = 2;              // function code + MsgType
constexpr size_t kChatHeaderSize = kUint32Size + 1;  // from_id + recipient_tag
constexpr uint16_t kMaxU16Count = 0xFFFF;

// Chat recipient_tag values.
constexpr uint8_t kRecipientById = 0x01;
constexpr uint8_t kRecipientByNickname = 0x02;
constexpr uint8_t kRecipientBroadcast = 0x03;  // empty recipient_data — deliver to all peers

constexpr const char* kBroadcastRecipient = "all";

struct RegisterPayload {
    std::string nickname;
};

struct AssignIdPayload {
    uint32_t id{0};
    bool create_room{false};
};

struct ChatPayload {
    uint32_t from_id{0};
    uint8_t recipient_tag{0};
    std::vector<uint8_t> recipient_data;
    std::string text;  // opaque body (plaintext historically; ciphertext for E2E)
};

struct DeliverPayload {
    uint32_t from_id{0};
    std::string text;  // opaque body
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

struct BannedIdsPayload {
    std::vector<uint32_t> ids;
};

using AppPayload = std::variant<RegisterPayload, AssignIdPayload, ChatPayload, DeliverPayload, ErrorPayload,
                                UserJoinedPayload, UserLeftPayload, ListUsersPayload, UsersListPayload,
                                BannedIdsPayload>;

struct AppMessage {
    MsgType type{};
    AppPayload payload;
};

inline uint32_t read_u32_be(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
}

inline void write_u32_be(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

inline std::vector<uint8_t> encode_u32_be(uint32_t value) {
    std::vector<uint8_t> out;
    write_u32_be(out, value);
    return out;
}

inline uint16_t read_u16_be(const uint8_t* data) {
    return (static_cast<uint16_t>(data[0]) << 8) | data[1];
}

inline void write_u16_be(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

std::vector<uint8_t> encode_string(const std::string& value);
std::string decode_string(const uint8_t* data, size_t len);

modbus::Frame make_frame(MsgType type, const std::vector<uint8_t>& payload, uint16_t transaction_id = 0);
AppMessage decode_app_message(const modbus::Frame& frame);

std::vector<uint8_t> encode_register(const std::string& nickname);
std::vector<uint8_t> encode_assign_id(uint32_t id, bool create_room = false);
std::vector<uint8_t> encode_chat(const ChatPayload& chat);
std::vector<uint8_t> encode_deliver(uint32_t from_id, const std::string& text);
std::vector<uint8_t> encode_error(const std::string& text);
std::vector<uint8_t> encode_user_joined(uint32_t id, const std::string& nickname);
std::vector<uint8_t> encode_user_left(uint32_t id, const std::string& nickname);
std::vector<uint8_t> encode_list_users();
std::vector<uint8_t> encode_users_list(const std::vector<UserInfo>& users);
std::vector<uint8_t> encode_banned_ids(const std::vector<uint32_t>& ids);

}  // namespace protocol
