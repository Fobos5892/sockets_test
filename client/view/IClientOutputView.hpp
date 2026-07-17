#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Protocol.hpp"

class IClientOutputView {
public:
    virtual ~IClientOutputView() = default;

    virtual void show_status(const std::string& step, int percent) = 0;
    virtual void show_input_ready() = 0;
    virtual void show_input_prompt() = 0;
    virtual void show_connected(const std::string& server_ip, uint16_t port) = 0;
    virtual void show_my_id(uint32_t id) = 0;
    virtual void show_usage() = 0;
    virtual void show_delivered(uint32_t from_id, const std::string& text) = 0;
    virtual void show_user_joined(uint32_t id, const std::string& nickname) = 0;
    virtual void show_user_left(uint32_t id, const std::string& nickname) = 0;
    virtual void show_users_list(const std::vector<protocol::UserInfo>& users) = 0;
    virtual void show_error(const std::string& text) = 0;
    virtual void show_notice(const std::string& text) = 0;
    virtual void show_send_failed() = 0;
    virtual void show_invalid_format() = 0;
    virtual void show_decode_error(const std::string& text) = 0;
    virtual void show_exiting() = 0;
    virtual void show_server_disconnected() = 0;
};
