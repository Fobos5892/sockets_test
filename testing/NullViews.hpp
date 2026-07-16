#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Protocol.hpp"
#include "client/view/IClientOutputView.hpp"
#include "server/view/IServerView.hpp"

class NullServerView final : public IServerView {
public:
    void show_listening(const std::string&, uint16_t) override {}
    void show_accept_failed(const std::string&) override {}
    void show_client_connected(uint32_t) override {}
    void show_client_disconnected(uint32_t, const std::string&) override {}
    void show_client_registered(uint32_t, const std::string&) override {}
    void show_chat(uint32_t, const std::string&, const std::string&) override {}
    void show_decode_error(const std::string&) override {}
};

class RecordingClientOutputView final : public IClientOutputView {
public:
    void show_connected(const std::string&, uint16_t) override {}
    void show_my_id(uint32_t) override {}
    void show_usage() override {}

    void show_delivered(uint32_t from_id, const std::string& text) override {
        last_deliver_from = from_id;
        last_deliver_text = text;
        ++deliver_count;
    }

    void show_user_joined(uint32_t id, const std::string& nickname) override {
        last_joined_id = id;
        last_joined_nickname = nickname;
        ++joined_count;
    }

    void show_user_left(uint32_t id, const std::string& nickname) override {
        last_left_id = id;
        last_left_nickname = nickname;
        ++left_count;
    }

    void show_users_list(const std::vector<protocol::UserInfo>& users) override {
        last_users = users;
        ++users_list_count;
    }

    void show_error(const std::string& text) override {
        last_error = text;
        ++error_count;
    }

    void show_send_failed() override {}
    void show_invalid_format() override {}
    void show_decode_error(const std::string&) override {}

    uint32_t last_deliver_from{0};
    std::string last_deliver_text;
    int deliver_count{0};

    uint32_t last_joined_id{0};
    std::string last_joined_nickname;
    int joined_count{0};

    uint32_t last_left_id{0};
    std::string last_left_nickname;
    int left_count{0};

    std::vector<protocol::UserInfo> last_users;
    int users_list_count{0};

    std::string last_error;
    int error_count{0};
};

class NullClientOutputView final : public IClientOutputView {
public:
    void show_connected(const std::string&, uint16_t) override {}
    void show_my_id(uint32_t) override {}
    void show_usage() override {}
    void show_delivered(uint32_t, const std::string&) override {}
    void show_user_joined(uint32_t, const std::string&) override {}
    void show_user_left(uint32_t, const std::string&) override {}
    void show_users_list(const std::vector<protocol::UserInfo>&) override {}
    void show_error(const std::string&) override {}
    void show_send_failed() override {}
    void show_invalid_format() override {}
    void show_decode_error(const std::string&) override {}
};
