#pragma once

#include <memory>
#include <vector>

#include "Protocol.hpp"
#include "viewmodel/IMessageHandler.hpp"
#include "viewmodel/ServerContext.hpp"

class MessageDispatcher {
public:
    void add_handler(std::unique_ptr<IMessageHandler> handler);
    void dispatch(int fd, const protocol::AppMessage& message, ServerContext& context) const;

private:
    std::vector<std::unique_ptr<IMessageHandler>> handlers_;
};
