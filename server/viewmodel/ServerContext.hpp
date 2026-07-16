#pragma once

#include "model/IClientRegistry.hpp"
#include "transport/IMessageTransport.hpp"
#include "view/IServerView.hpp"
#include "viewmodel/PresenceBroadcaster.hpp"

struct ServerContext {
    IServerView& view;
    server_model::IClientRegistry& registry;
    IMessageTransport& transport;
    PresenceBroadcaster& presence;
};
