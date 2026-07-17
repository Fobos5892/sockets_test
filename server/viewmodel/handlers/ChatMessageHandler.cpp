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

    if (context.is_banned(sender->id)) {
        return;
    }

    const uint32_t from_id = sender->id;
    const std::string to_label = context.registry.recipient_label(chat);
    // Blind relay: never log ciphertext/plaintext body.
    context.view.show_chat(from_id, to_label, "Chat");

    if (chat.recipient_tag == protocol::kRecipientBroadcast) {
        const auto deliver = protocol::encode_deliver(from_id, chat.text);
        for (const int target_fd : context.registry.all_fds()) {
            if (target_fd == fd) {
                continue;
            }
            const auto* target = context.registry.find_by_fd(target_fd);
            if (!target || context.is_banned(target->id)) {
                continue;
            }
            context.transport.send(target_fd, protocol::MsgType::Deliver, deliver);
        }
        return;
    }

    const auto target_id = context.registry.resolve_recipient(chat);
    if (!target_id) {
        context.transport.send(fd, protocol::MsgType::Error,
                               protocol::encode_error("Unknown recipient: " + to_label));
        return;
    }
    if (context.is_banned(*target_id)) {
        context.transport.send(fd, protocol::MsgType::Error,
                               protocol::encode_error("Recipient banned: " + to_label));
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
