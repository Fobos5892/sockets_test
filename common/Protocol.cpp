#include "Protocol.hpp"

#include <stdexcept>

namespace protocol {

namespace {

UserInfo decode_user_info(const uint8_t* data, size_t len, size_t& offset) {
    if (len < offset + kUint32Size) {
        throw std::runtime_error("UserInfo id too short");
    }
    UserInfo info;
    info.id = read_u32_be(data + offset);
    offset += kUint32Size;
    info.nickname = decode_string(data + offset, len - offset);
    offset += kUint16Size + info.nickname.size();
    return info;
}

std::vector<uint8_t> encode_user_info(const UserInfo& info) {
    std::vector<uint8_t> out;
    write_u32_be(out, info.id);
    const auto nickname_bytes = encode_string(info.nickname);
    out.insert(out.end(), nickname_bytes.begin(), nickname_bytes.end());
    return out;
}

ChatPayload decode_chat_payload(const uint8_t* payload, size_t payload_len) {
    if (payload_len < kChatHeaderSize) {
        throw std::runtime_error("Chat payload too short");
    }
    ChatPayload chat;
    chat.from_id = read_u32_be(payload);
    chat.recipient_tag = payload[kUint32Size];
    size_t offset = kChatHeaderSize;

    if (chat.recipient_tag == kRecipientById) {
        if (payload_len < offset + kUint32Size) {
            throw std::runtime_error("Chat id recipient too short");
        }
        chat.recipient_data.assign(payload + offset, payload + offset + kUint32Size);
        offset += kUint32Size;
    } else if (chat.recipient_tag == kRecipientByNickname) {
        const std::string nickname = decode_string(payload + offset, payload_len - offset);
        chat.recipient_data = encode_string(nickname);
        offset += chat.recipient_data.size();
    } else if (chat.recipient_tag == kRecipientBroadcast) {
        // No recipient bytes; body follows immediately.
    } else {
        throw std::runtime_error("Unknown recipient tag");
    }

    chat.text = decode_string(payload + offset, payload_len - offset);
    return chat;
}

DeliverPayload decode_deliver_payload(const uint8_t* payload, size_t payload_len) {
    if (payload_len < kUint32Size) {
        throw std::runtime_error("Deliver payload too short");
    }
    DeliverPayload deliver;
    deliver.from_id = read_u32_be(payload);
    deliver.text = decode_string(payload + kUint32Size, payload_len - kUint32Size);
    return deliver;
}

}  // namespace

std::vector<uint8_t> encode_string(const std::string& value) {
    if (value.size() > kMaxU16Count) {
        throw std::runtime_error("String too long");
    }
    std::vector<uint8_t> out;
    write_u16_be(out, static_cast<uint16_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
    return out;
}

std::string decode_string(const uint8_t* data, size_t len) {
    if (len < kUint16Size) {
        throw std::runtime_error("Invalid string payload");
    }
    const uint16_t str_len = read_u16_be(data);
    if (len < kUint16Size + str_len) {
        throw std::runtime_error("Truncated string payload");
    }
    return std::string(reinterpret_cast<const char*>(data + kUint16Size), str_len);
}

modbus::Frame make_frame(MsgType type, const std::vector<uint8_t>& payload, uint16_t transaction_id) {
    modbus::Frame frame;
    frame.transaction_id = transaction_id;
    frame.unit_id = 0;
    frame.pdu.push_back(modbus::kAppFunctionCode);
    frame.pdu.push_back(static_cast<uint8_t>(type));
    frame.pdu.insert(frame.pdu.end(), payload.begin(), payload.end());
    return frame;
}

AppMessage decode_app_message(const modbus::Frame& frame) {
    if (frame.pdu.size() < kPduAppHeaderSize) {
        throw std::runtime_error("PDU too short");
    }
    if (frame.pdu[0] != modbus::kAppFunctionCode) {
        throw std::runtime_error("Unexpected function code");
    }

    AppMessage message;
    message.type = static_cast<MsgType>(frame.pdu[1]);
    const uint8_t* payload = frame.pdu.data() + kPduAppHeaderSize;
    const size_t payload_len = frame.pdu.size() - kPduAppHeaderSize;

    switch (message.type) {
        case MsgType::Register: {
            RegisterPayload reg;
            reg.nickname = decode_string(payload, payload_len);
            message.payload = reg;
            break;
        }
        case MsgType::AssignId: {
            if (payload_len < kUint32Size) {
                throw std::runtime_error("AssignId payload too short");
            }
            AssignIdPayload assign;
            assign.id = read_u32_be(payload);
            assign.create_room = payload_len > kUint32Size && payload[kUint32Size] != 0;
            message.payload = assign;
            break;
        }
        case MsgType::Chat:
        case MsgType::KeyOffer:
        case MsgType::RoomKeyOffer: {
            message.payload = decode_chat_payload(payload, payload_len);
            break;
        }
        case MsgType::Deliver:
        case MsgType::PeerKey:
        case MsgType::RoomKey: {
            message.payload = decode_deliver_payload(payload, payload_len);
            break;
        }
        case MsgType::Error: {
            ErrorPayload error;
            error.text = decode_string(payload, payload_len);
            message.payload = error;
            break;
        }
        case MsgType::UserJoined: {
            size_t offset = 0;
            const UserInfo info = decode_user_info(payload, payload_len, offset);
            message.payload = UserJoinedPayload{info.id, info.nickname};
            break;
        }
        case MsgType::UserLeft: {
            size_t offset = 0;
            const UserInfo info = decode_user_info(payload, payload_len, offset);
            message.payload = UserLeftPayload{info.id, info.nickname};
            break;
        }
        case MsgType::ListUsers: {
            message.payload = ListUsersPayload{};
            break;
        }
        case MsgType::UsersList: {
            if (payload_len < kUint16Size) {
                throw std::runtime_error("UsersList payload too short");
            }
            UsersListPayload list;
            const uint16_t count = read_u16_be(payload);
            size_t offset = kUint16Size;
            list.users.reserve(count);
            for (uint16_t i = 0; i < count; ++i) {
                list.users.push_back(decode_user_info(payload, payload_len, offset));
            }
            message.payload = list;
            break;
        }
        case MsgType::BannedIds: {
            if (payload_len < kUint16Size) {
                throw std::runtime_error("BannedIds payload too short");
            }
            BannedIdsPayload banned;
            const uint16_t count = read_u16_be(payload);
            size_t offset = kUint16Size;
            if (payload_len < offset + static_cast<size_t>(count) * kUint32Size) {
                throw std::runtime_error("BannedIds payload truncated");
            }
            banned.ids.reserve(count);
            for (uint16_t i = 0; i < count; ++i) {
                banned.ids.push_back(read_u32_be(payload + offset));
                offset += kUint32Size;
            }
            message.payload = banned;
            break;
        }
        default:
            throw std::runtime_error("Unknown message type");
    }

    return message;
}

std::vector<uint8_t> encode_register(const std::string& nickname) {
    return encode_string(nickname);
}

std::vector<uint8_t> encode_assign_id(uint32_t id, bool create_room) {
    auto out = encode_u32_be(id);
    out.push_back(create_room ? 1 : 0);
    return out;
}

std::vector<uint8_t> encode_chat(const ChatPayload& chat) {
    std::vector<uint8_t> out;
    write_u32_be(out, chat.from_id);
    out.push_back(chat.recipient_tag);
    out.insert(out.end(), chat.recipient_data.begin(), chat.recipient_data.end());
    const auto text_bytes = encode_string(chat.text);
    out.insert(out.end(), text_bytes.begin(), text_bytes.end());
    return out;
}

std::vector<uint8_t> encode_deliver(uint32_t from_id, const std::string& text) {
    std::vector<uint8_t> out;
    write_u32_be(out, from_id);
    const auto text_bytes = encode_string(text);
    out.insert(out.end(), text_bytes.begin(), text_bytes.end());
    return out;
}

std::vector<uint8_t> encode_error(const std::string& text) {
    return encode_string(text);
}

std::vector<uint8_t> encode_user_joined(uint32_t id, const std::string& nickname) {
    return encode_user_info(UserInfo{id, nickname});
}

std::vector<uint8_t> encode_user_left(uint32_t id, const std::string& nickname) {
    return encode_user_info(UserInfo{id, nickname});
}

std::vector<uint8_t> encode_list_users() {
    return {};
}

std::vector<uint8_t> encode_users_list(const std::vector<UserInfo>& users) {
    if (users.size() > kMaxU16Count) {
        throw std::runtime_error("Too many users");
    }
    std::vector<uint8_t> out;
    write_u16_be(out, static_cast<uint16_t>(users.size()));
    for (const auto& user : users) {
        const auto bytes = encode_user_info(user);
        out.insert(out.end(), bytes.begin(), bytes.end());
    }
    return out;
}

std::vector<uint8_t> encode_banned_ids(const std::vector<uint32_t>& ids) {
    if (ids.size() > kMaxU16Count) {
        throw std::runtime_error("Too many banned ids");
    }
    std::vector<uint8_t> out;
    write_u16_be(out, static_cast<uint16_t>(ids.size()));
    for (const uint32_t id : ids) {
        write_u32_be(out, id);
    }
    return out;
}

}  // namespace protocol
