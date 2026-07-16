#include "Protocol.hpp"

#include <stdexcept>

namespace protocol {

namespace {

uint32_t read_u32_be(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           static_cast<uint32_t>(data[3]);
}

void write_u32_be(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

uint16_t read_u16_be(const uint8_t* data) {
    return (static_cast<uint16_t>(data[0]) << 8) | data[1];
}

void write_u16_be(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

UserInfo decode_user_info(const uint8_t* data, size_t len, size_t& offset) {
    if (len < offset + 4) {
        throw std::runtime_error("UserInfo id too short");
    }
    UserInfo info;
    info.id = read_u32_be(data + offset);
    offset += 4;
    info.nickname = decode_string(data + offset, len - offset);
    offset += 2 + info.nickname.size();
    return info;
}

std::vector<uint8_t> encode_user_info(const UserInfo& info) {
    std::vector<uint8_t> out;
    write_u32_be(out, info.id);
    const auto nickname_bytes = encode_string(info.nickname);
    out.insert(out.end(), nickname_bytes.begin(), nickname_bytes.end());
    return out;
}

}  // namespace

std::vector<uint8_t> encode_string(const std::string& value) {
    if (value.size() > 0xFFFF) {
        throw std::runtime_error("String too long");
    }
    std::vector<uint8_t> out;
    const uint16_t len = static_cast<uint16_t>(value.size());
    out.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(len & 0xFF));
    out.insert(out.end(), value.begin(), value.end());
    return out;
}

std::string decode_string(const uint8_t* data, size_t len) {
    if (len < 2) {
        throw std::runtime_error("Invalid string payload");
    }
    const uint16_t str_len = (static_cast<uint16_t>(data[0]) << 8) | data[1];
    if (len < 2 + str_len) {
        throw std::runtime_error("Truncated string payload");
    }
    return std::string(reinterpret_cast<const char*>(data + 2), str_len);
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
    if (frame.pdu.size() < 2) {
        throw std::runtime_error("PDU too short");
    }
    if (frame.pdu[0] != modbus::kAppFunctionCode) {
        throw std::runtime_error("Unexpected function code");
    }

    AppMessage message;
    message.type = static_cast<MsgType>(frame.pdu[1]);
    const uint8_t* payload = frame.pdu.data() + 2;
    const size_t payload_len = frame.pdu.size() - 2;

    switch (message.type) {
        case MsgType::Register: {
            RegisterPayload reg;
            reg.nickname = decode_string(payload, payload_len);
            message.payload = reg;
            break;
        }
        case MsgType::AssignId: {
            if (payload_len < 4) {
                throw std::runtime_error("AssignId payload too short");
            }
            AssignIdPayload assign;
            assign.id = read_u32_be(payload);
            message.payload = assign;
            break;
        }
        case MsgType::Chat: {
            if (payload_len < 5) {
                throw std::runtime_error("Chat payload too short");
            }
            ChatPayload chat;
            chat.from_id = read_u32_be(payload);
            chat.recipient_tag = payload[4];
            size_t offset = 5;

            if (chat.recipient_tag == 0x01) {
                if (payload_len < offset + 4) {
                    throw std::runtime_error("Chat id recipient too short");
                }
                chat.recipient_data.assign(payload + offset, payload + offset + 4);
                offset += 4;
            } else if (chat.recipient_tag == 0x02) {
                const std::string nickname = decode_string(payload + offset, payload_len - offset);
                chat.recipient_data = encode_string(nickname);
                offset += chat.recipient_data.size();
            } else {
                throw std::runtime_error("Unknown recipient tag");
            }

            chat.text = decode_string(payload + offset, payload_len - offset);
            message.payload = chat;
            break;
        }
        case MsgType::Deliver: {
            if (payload_len < 4) {
                throw std::runtime_error("Deliver payload too short");
            }
            DeliverPayload deliver;
            deliver.from_id = read_u32_be(payload);
            deliver.text = decode_string(payload + 4, payload_len - 4);
            message.payload = deliver;
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
            if (payload_len < 2) {
                throw std::runtime_error("UsersList payload too short");
            }
            UsersListPayload list;
            const uint16_t count = read_u16_be(payload);
            size_t offset = 2;
            list.users.reserve(count);
            for (uint16_t i = 0; i < count; ++i) {
                list.users.push_back(decode_user_info(payload, payload_len, offset));
            }
            message.payload = list;
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

std::vector<uint8_t> encode_assign_id(uint32_t id) {
    std::vector<uint8_t> out;
    write_u32_be(out, id);
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
    if (users.size() > 0xFFFF) {
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

}  // namespace protocol
