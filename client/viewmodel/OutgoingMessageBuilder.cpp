#include "viewmodel/OutgoingMessageBuilder.hpp"

namespace client_viewmodel {

namespace {

std::vector<uint8_t> build_chat_like(uint32_t from_id, uint8_t recipient_tag, std::vector<uint8_t> recipient_data,
                                     const std::string& text) {
    protocol::ChatPayload chat;
    chat.from_id = from_id;
    chat.recipient_tag = recipient_tag;
    chat.recipient_data = std::move(recipient_data);
    chat.text = text;
    return protocol::encode_chat(chat);
}

}  // namespace

std::vector<uint8_t> OutgoingMessageBuilder::build_chat_by_id(uint32_t from_id, uint32_t recipient_id,
                                                              const std::string& text) {
    return build_chat_like(from_id, protocol::kRecipientById, protocol::encode_u32_be(recipient_id), text);
}

std::vector<uint8_t> OutgoingMessageBuilder::build_chat_by_nickname(uint32_t from_id, const std::string& recipient,
                                                                    const std::string& text) {
    return build_chat_like(from_id, protocol::kRecipientByNickname, protocol::encode_string(recipient), text);
}

std::vector<uint8_t> OutgoingMessageBuilder::build_chat_broadcast(uint32_t from_id, const std::string& text) {
    return build_chat_like(from_id, protocol::kRecipientBroadcast, {}, text);
}

std::vector<uint8_t> OutgoingMessageBuilder::build_key_offer(uint32_t from_id, uint32_t recipient_id,
                                                             const std::string& body) {
    return build_chat_like(from_id, protocol::kRecipientById, protocol::encode_u32_be(recipient_id), body);
}

std::vector<uint8_t> OutgoingMessageBuilder::build_room_key_offer(uint32_t from_id, uint32_t recipient_id,
                                                                  const std::string& body) {
    return build_chat_like(from_id, protocol::kRecipientById, protocol::encode_u32_be(recipient_id), body);
}

std::vector<uint8_t> OutgoingMessageBuilder::build_list_users() {
    return protocol::encode_list_users();
}

}  // namespace client_viewmodel
