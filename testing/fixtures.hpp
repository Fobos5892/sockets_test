#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Protocol.hpp"

namespace testing_fixtures {

inline protocol::ChatPayload make_chat_by_id(uint32_t from_id, uint32_t recipient_id, const std::string& text) {
    protocol::ChatPayload chat;
    chat.from_id = from_id;
    chat.recipient_tag = protocol::kRecipientById;
    chat.recipient_data = protocol::encode_u32_be(recipient_id);
    chat.text = text;
    return chat;
}

inline protocol::ChatPayload make_chat_by_nickname(uint32_t from_id, const std::string& nickname,
                                                   const std::string& text) {
    protocol::ChatPayload chat;
    chat.from_id = from_id;
    chat.recipient_tag = protocol::kRecipientByNickname;
    chat.recipient_data = protocol::encode_string(nickname);
    chat.text = text;
    return chat;
}

inline protocol::ChatPayload make_chat_broadcast(uint32_t from_id, const std::string& text) {
    protocol::ChatPayload chat;
    chat.from_id = from_id;
    chat.recipient_tag = protocol::kRecipientBroadcast;
    chat.text = text;
    return chat;
}

inline protocol::AppMessage make_chat_message(const protocol::ChatPayload& chat) {
    return protocol::AppMessage{protocol::MsgType::Chat, chat};
}

inline protocol::AppMessage make_register_message(const std::string& nickname) {
    return protocol::AppMessage{protocol::MsgType::Register, protocol::RegisterPayload{nickname}};
}

inline protocol::AppMessage make_list_users_message() {
    return protocol::AppMessage{protocol::MsgType::ListUsers, protocol::ListUsersPayload{}};
}

inline protocol::AppMessage make_deliver_message(uint32_t from_id, const std::string& text) {
    return protocol::AppMessage{protocol::MsgType::Deliver, protocol::DeliverPayload{from_id, text}};
}

}  // namespace testing_fixtures
