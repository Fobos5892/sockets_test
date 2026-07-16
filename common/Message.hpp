#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "Protocol.hpp"

template <typename T>
struct RecipientTraits;

template <>
struct RecipientTraits<uint32_t> {
    static constexpr uint8_t tag = 0x01;

    static std::vector<uint8_t> serialize(uint32_t value) {
        std::vector<uint8_t> out(4);
        out[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
        out[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
        out[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
        out[3] = static_cast<uint8_t>(value & 0xFF);
        return out;
    }

    static uint32_t deserialize(const uint8_t* data, size_t len) {
        if (len < 4) {
            throw std::runtime_error("Invalid id recipient payload");
        }
        return (static_cast<uint32_t>(data[0]) << 24) |
               (static_cast<uint32_t>(data[1]) << 16) |
               (static_cast<uint32_t>(data[2]) << 8) |
               static_cast<uint32_t>(data[3]);
    }
};

template <>
struct RecipientTraits<std::string> {
    static constexpr uint8_t tag = 0x02;

    static std::vector<uint8_t> serialize(const std::string& value) {
        return protocol::encode_string(value);
    }

    static std::string deserialize(const uint8_t* data, size_t len) {
        return protocol::decode_string(data, len);
    }
};

template <typename PayloadT>
struct PayloadTraits;

template <>
struct PayloadTraits<std::string> {
    static std::vector<uint8_t> serialize(const std::string& value) {
        return protocol::encode_string(value);
    }

    static std::string deserialize(const uint8_t* data, size_t len) {
        return protocol::decode_string(data, len);
    }
};

template <typename RecipientT, typename PayloadT = std::string>
class Message {
public:
    RecipientT recipient{};
    PayloadT payload{};
    uint32_t from_id{0};

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> out;
        out.push_back(RecipientTraits<RecipientT>::tag);
        const auto recipient_bytes = RecipientTraits<RecipientT>::serialize(recipient);
        out.insert(out.end(), recipient_bytes.begin(), recipient_bytes.end());
        const auto payload_bytes = PayloadTraits<PayloadT>::serialize(payload);
        out.insert(out.end(), payload_bytes.begin(), payload_bytes.end());
        return out;
    }

    static Message deserialize(const uint8_t* data, size_t len) {
        if (len < 1) {
            throw std::runtime_error("Empty message payload");
        }

        const uint8_t tag = data[0];
        if (tag != RecipientTraits<RecipientT>::tag) {
            throw std::runtime_error("Recipient tag mismatch");
        }

        Message message;
        size_t offset = 1;

        if constexpr (std::is_same_v<RecipientT, uint32_t>) {
            message.recipient = RecipientTraits<uint32_t>::deserialize(data + offset, len - offset);
            offset += 4;
        } else {
            message.recipient = RecipientTraits<std::string>::deserialize(data + offset, len - offset);
            const uint16_t str_len = (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
            offset += 2 + str_len;
        }

        message.payload = PayloadTraits<PayloadT>::deserialize(data + offset, len - offset);
        return message;
    }
};

inline bool is_numeric_recipient(const std::string& text) {
    if (text.empty()) {
        return false;
    }
    for (char ch : text) {
        if (ch < '0' || ch > '9') {
            return false;
        }
    }
    return true;
}

inline std::optional<Message<uint32_t>> parse_id_message(const std::string& line, uint32_t from_id) {
    const auto pos = line.find(':');
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    const std::string recipient = line.substr(0, pos);
    if (!is_numeric_recipient(recipient)) {
        return std::nullopt;
    }

    Message<uint32_t> message;
    message.from_id = from_id;
    message.recipient = static_cast<uint32_t>(std::stoul(recipient));
    message.payload = line.substr(pos + 1);
    return message;
}

inline std::optional<Message<std::string>> parse_nickname_message(const std::string& line, uint32_t from_id) {
    const auto pos = line.find(':');
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    const std::string recipient = line.substr(0, pos);
    if (is_numeric_recipient(recipient)) {
        return std::nullopt;
    }

    Message<std::string> message;
    message.from_id = from_id;
    message.recipient = recipient;
    message.payload = line.substr(pos + 1);
    return message;
}
