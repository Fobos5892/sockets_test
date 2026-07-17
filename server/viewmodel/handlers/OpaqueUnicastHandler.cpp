#include "viewmodel/handlers/OpaqueUnicastHandler.hpp"

#include "model/ClientRecord.hpp"

OpaqueUnicastHandler::OpaqueUnicastHandler(protocol::MsgType request_type, protocol::MsgType deliver_type,
                                           const char* log_label)
    : request_type_(request_type), deliver_type_(deliver_type), log_label_(log_label) {}

bool OpaqueUnicastHandler::can_handle(protocol::MsgType type) const {
    return type == request_type_;
}

void OpaqueUnicastHandler::handle(int fd, const protocol::AppMessage& message, ServerContext& context) {
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
    context.view.show_chat(from_id, to_label, log_label_);

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
            context.transport.send(target_fd, deliver_type_, deliver);
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

    context.transport.send(*target_fd, deliver_type_, protocol::encode_deliver(from_id, chat.text));
}
