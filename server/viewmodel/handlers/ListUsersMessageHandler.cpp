#include "viewmodel/handlers/ListUsersMessageHandler.hpp"

bool ListUsersMessageHandler::can_handle(protocol::MsgType type) const {
    return type == protocol::MsgType::ListUsers;
}

void ListUsersMessageHandler::handle(int fd, const protocol::AppMessage& message, ServerContext& context) {
    (void)message;
    context.transport.send(fd, protocol::MsgType::UsersList,
                           protocol::encode_users_list(context.registry.collect_users()));
}
