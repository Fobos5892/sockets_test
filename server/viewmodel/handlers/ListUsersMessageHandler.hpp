#pragma once

#include "viewmodel/IMessageHandler.hpp"

class ListUsersMessageHandler final : public IMessageHandler {
public:
    bool can_handle(protocol::MsgType type) const override;
    void handle(int fd, const protocol::AppMessage& message, ServerContext& context) override;
};
