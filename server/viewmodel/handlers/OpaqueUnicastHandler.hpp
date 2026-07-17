#pragma once

#include "viewmodel/IMessageHandler.hpp"

// Forwards KeyOffer / RoomKeyOffer (ChatPayload) as PeerKey / RoomKey (DeliverPayload).
class OpaqueUnicastHandler final : public IMessageHandler {
public:
    OpaqueUnicastHandler(protocol::MsgType request_type, protocol::MsgType deliver_type, const char* log_label);

    bool can_handle(protocol::MsgType type) const override;
    void handle(int fd, const protocol::AppMessage& message, ServerContext& context) override;

private:
    protocol::MsgType request_type_;
    protocol::MsgType deliver_type_;
    const char* log_label_;
};
