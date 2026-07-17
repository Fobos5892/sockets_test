#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Protocol.hpp"

namespace client_viewmodel {

class OutgoingMessageBuilder {
public:
    static std::vector<uint8_t> build_chat_by_id(uint32_t from_id, uint32_t recipient_id, const std::string& text);
    static std::vector<uint8_t> build_chat_by_nickname(uint32_t from_id, const std::string& recipient,
                                                       const std::string& text);
    static std::vector<uint8_t> build_chat_broadcast(uint32_t from_id, const std::string& text);
    static std::vector<uint8_t> build_key_offer(uint32_t from_id, uint32_t recipient_id, const std::string& body);
    static std::vector<uint8_t> build_room_key_offer(uint32_t from_id, uint32_t recipient_id, const std::string& body);
    static std::vector<uint8_t> build_list_users();
};

}  // namespace client_viewmodel
