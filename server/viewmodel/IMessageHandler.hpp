#pragma once

#include "Protocol.hpp"
#include "viewmodel/ServerContext.hpp"

class IMessageHandler {
public:
    virtual ~IMessageHandler() = default;

    virtual bool can_handle(protocol::MsgType type) const = 0;
    virtual void handle(int fd, const protocol::AppMessage& message, ServerContext& context) = 0;
};
