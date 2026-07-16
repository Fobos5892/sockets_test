#include "viewmodel/OutgoingMessageBuilder.hpp"

namespace client_viewmodel {

std::vector<uint8_t> OutgoingMessageBuilder::build_chat_by_id(uint32_t from_id, uint32_t recipient_id,
                                                              const std::string& text) {
    protocol::ChatPayload chat;
    chat.from_id = from_id;
    chat.recipient_tag = 0x01;
    chat.recipient_data = {
        static_cast<uint8_t>((recipient_id >> 24) & 0xFF),
        static_cast<uint8_t>((recipient_id >> 16) & 0xFF),
        static_cast<uint8_t>((recipient_id >> 8) & 0xFF),
        static_cast<uint8_t>(recipient_id & 0xFF),
    };
    chat.text = text;
    return protocol::encode_chat(chat);
}

std::vector<uint8_t> OutgoingMessageBuilder::build_chat_by_nickname(uint32_t from_id, const std::string& recipient,
                                                                    const std::string& text) {
    protocol::ChatPayload chat;
    chat.from_id = from_id;
    chat.recipient_tag = 0x02;
    chat.recipient_data = protocol::encode_string(recipient);
    chat.text = text;
    return protocol::encode_chat(chat);
}

std::vector<uint8_t> OutgoingMessageBuilder::build_list_users() {
    return protocol::encode_list_users();
}

}  // namespace client_viewmodel
