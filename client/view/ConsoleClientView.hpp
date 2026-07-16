#pragma once

#include "view/IClientView.hpp"

class ConsoleClientView final : public IClientView {
public:
    void show_connected(const std::string& server_ip, uint16_t port) override;
    void show_my_id(uint32_t id) override;
    void show_usage() override;
    std::optional<std::string> read_line() override;
    void show_delivered(uint32_t from_id, const std::string& text) override;
    void show_user_joined(uint32_t id, const std::string& nickname) override;
    void show_user_left(uint32_t id, const std::string& nickname) override;
    void show_users_list(const std::vector<protocol::UserInfo>& users) override;
    void show_error(const std::string& text) override;
    void show_send_failed() override;
    void show_invalid_format() override;
    void show_decode_error(const std::string& text) override;
};
