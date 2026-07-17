#include "viewmodel/handlers/RegisterMessageHandler.hpp"

#include "model/ClientRecord.hpp"

bool RegisterMessageHandler::can_handle(protocol::MsgType type) const {
    return type == protocol::MsgType::Register;
}

void RegisterMessageHandler::handle(int fd, const protocol::AppMessage& message, ServerContext& context) {
    const auto& nickname = std::get<protocol::RegisterPayload>(message.payload).nickname;
    const auto* record = context.registry.find_by_fd(fd);
    if (!record) {
        return;
    }

    if (nickname.empty()) {
        context.transport.send(fd, protocol::MsgType::Error, protocol::encode_error("Nickname cannot be empty"));
        return;
    }

    if (nickname == protocol::kBroadcastRecipient) {
        context.transport.send(fd, protocol::MsgType::Error,
                               protocol::encode_error("Nickname 'all' is reserved"));
        return;
    }

    if (context.registry.nickname_taken_by_other(nickname, record->id)) {
        context.transport.send(fd, protocol::MsgType::Error, protocol::encode_error("Nickname already taken"));
        return;
    }

    context.registry.set_nickname(fd, nickname);
    context.view.show_client_registered(record->id, nickname);
    context.presence.user_joined(record->id, nickname, fd);
}
