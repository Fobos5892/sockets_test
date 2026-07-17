#pragma once

#include "view/IServerView.hpp"

class ServerLogView final : public IServerView {
public:
    void show_listening(const std::string& bind_ip, uint16_t port) override;
    void show_accept_failed(const std::string& error) override;
    void show_client_connected(uint32_t id) override;
    void show_client_disconnected(uint32_t id, const std::string& nickname) override;
    void show_client_registered(uint32_t id, const std::string& nickname) override;
    void show_chat(uint32_t from_id, const std::string& to_label, const std::string& kind) override;
    void show_decode_error(const std::string& error) override;
    void show_admin_message(const std::string& text) override;
};
