#pragma once

#include <cstdint>
#include <string>

class IServerView {
public:
    virtual ~IServerView() = default;

    virtual void show_listening(const std::string& bind_ip, uint16_t port) = 0;
    virtual void show_accept_failed(const std::string& error) = 0;
    virtual void show_client_connected(uint32_t id) = 0;
    virtual void show_client_disconnected(uint32_t id, const std::string& nickname) = 0;
    virtual void show_client_registered(uint32_t id, const std::string& nickname) = 0;
    // kind is a short label only (e.g. "Chat"); body must never be logged (E2E blind relay).
    virtual void show_chat(uint32_t from_id, const std::string& to_label, const std::string& kind) = 0;
    virtual void show_decode_error(const std::string& error) = 0;
    virtual void show_admin_message(const std::string& text) = 0;
};
