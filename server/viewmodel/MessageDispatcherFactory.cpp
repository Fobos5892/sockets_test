#include "viewmodel/MessageDispatcherFactory.hpp"

#include "viewmodel/handlers/ChatMessageHandler.hpp"
#include "viewmodel/handlers/ListUsersMessageHandler.hpp"
#include "viewmodel/handlers/RegisterMessageHandler.hpp"

std::unique_ptr<MessageDispatcher> create_message_dispatcher() {
    auto dispatcher = std::make_unique<MessageDispatcher>();
    dispatcher->add_handler(std::make_unique<RegisterMessageHandler>());
    dispatcher->add_handler(std::make_unique<ChatMessageHandler>());
    dispatcher->add_handler(std::make_unique<ListUsersMessageHandler>());
    return dispatcher;
}
