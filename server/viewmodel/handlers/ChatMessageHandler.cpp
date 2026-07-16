#include "viewmodel/handlers/ChatMessageHandler.hpp"

#include "model/ClientRecord.hpp"

bool ChatMessageHandler::can_handle(protocol::MsgType type) const {
    return type == protocol::MsgType::Chat;
}

void ChatMessageHandler::handle(int fd, const protocol::AppMessage& message, ServerContext& context) {
    const auto& chat = std::get<protocol::ChatPayload>(message.payload);
    const auto* sender = context.registry.find_by_fd(fd);
    if (!sender) {
        return;
    }

    const uint32_t from_id = sender->id;
    const std::string to_label = context.registry.recipient_label(chat);
    context.view.show_chat(from_id, to_label, chat.text);

    const auto target_id = context.registry.resolve_recipient(chat);
    if (!target_id) {
        context.transport.send(fd, protocol::MsgType::Error,
                               protocol::encode_error("Unknown recipient: " + to_label));
        return;
    }

    const auto target_fd = context.registry.fd_by_id(*target_id);
    if (!target_fd) {
        context.transport.send(fd, protocol::MsgType::Error,
                               protocol::encode_error("Recipient offline: " + to_label));
        return;
    }

    context.transport.send(*target_fd, protocol::MsgType::Deliver, protocol::encode_deliver(from_id, chat.text));
}
