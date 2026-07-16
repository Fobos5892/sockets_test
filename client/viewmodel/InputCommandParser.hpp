#pragma once

#include <optional>
#include <string>

#include "Message.hpp"

namespace client_viewmodel {

enum class InputCommandType {
    ListUsers,
    ChatById,
    ChatByNickname,
    Invalid,
};

struct InputCommand {
    InputCommandType type{InputCommandType::Invalid};
    uint32_t recipient_id{0};
    std::string recipient_nickname;
    std::string text;
};

inline InputCommand parse_input_line(const std::string& line, uint32_t from_id) {
    if (line.empty()) {
        return {};
    }

    if (line == "/users") {
        InputCommand command;
        command.type = InputCommandType::ListUsers;
        return command;
    }

    if (auto id_message = parse_id_message(line, from_id)) {
        InputCommand command;
        command.type = InputCommandType::ChatById;
        command.recipient_id = id_message->recipient;
        command.text = id_message->payload;
        return command;
    }

    if (auto nick_message = parse_nickname_message(line, from_id)) {
        InputCommand command;
        command.type = InputCommandType::ChatByNickname;
        command.recipient_nickname = nick_message->recipient;
        command.text = nick_message->payload;
        return command;
    }

    InputCommand command;
    command.type = InputCommandType::Invalid;
    return command;
}

}  // namespace client_viewmodel
