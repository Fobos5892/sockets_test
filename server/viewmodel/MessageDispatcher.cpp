#include "viewmodel/MessageDispatcher.hpp"

void MessageDispatcher::add_handler(std::unique_ptr<IMessageHandler> handler) {
    handlers_.push_back(std::move(handler));
}

void MessageDispatcher::dispatch(int fd, const protocol::AppMessage& message, ServerContext& context) const {
    for (const auto& handler : handlers_) {
        if (handler->can_handle(message.type)) {
            handler->handle(fd, message, context);
            return;
        }
    }

    context.transport.send(fd, protocol::MsgType::Error,
                           protocol::encode_error("Unsupported message type"));
}
