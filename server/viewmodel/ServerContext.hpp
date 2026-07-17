#pragma once

#include "model/IClientRegistry.hpp"
#include "transport/IMessageTransport.hpp"
#include "view/IServerView.hpp"
#include "viewmodel/PresenceBroadcaster.hpp"

#include <cstdint>
#include <unordered_set>

struct ServerContext {
    IServerView& view;
    server_model::IClientRegistry& registry;
    IMessageTransport& transport;
    PresenceBroadcaster& presence;
    const std::unordered_set<uint32_t>* banned_ids{nullptr};

    bool is_banned(uint32_t id) const {
        return banned_ids != nullptr && banned_ids->find(id) != banned_ids->end();
    }
};
