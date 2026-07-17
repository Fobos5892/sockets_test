#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "Protocol.hpp"
#include "view/IClientOutputCommand.hpp"
#include "view/IClientOutputView.hpp"

namespace client_view {

class ShowConnectedCommand final : public IClientOutputCommand {
public:
    ShowConnectedCommand(std::string server_ip, uint16_t port)
        : server_ip_(std::move(server_ip)), port_(port) {}

    void execute(IClientOutputView& view) override { view.show_connected(server_ip_, port_); }

private:
    std::string server_ip_;
    uint16_t port_;
};

class ShowMyIdCommand final : public IClientOutputCommand {
public:
    explicit ShowMyIdCommand(uint32_t id) : id_(id) {}

    void execute(IClientOutputView& view) override { view.show_my_id(id_); }

private:
    uint32_t id_;
};

class ShowUsageCommand final : public IClientOutputCommand {
public:
    void execute(IClientOutputView& view) override { view.show_usage(); }
};

class ShowDeliveredCommand final : public IClientOutputCommand {
public:
    ShowDeliveredCommand(uint32_t from_id, std::string text)
        : from_id_(from_id), text_(std::move(text)) {}

    void execute(IClientOutputView& view) override { view.show_delivered(from_id_, text_); }

private:
    uint32_t from_id_;
    std::string text_;
};

class ShowUserJoinedCommand final : public IClientOutputCommand {
public:
    ShowUserJoinedCommand(uint32_t id, std::string nickname)
        : id_(id), nickname_(std::move(nickname)) {}

    void execute(IClientOutputView& view) override { view.show_user_joined(id_, nickname_); }

private:
    uint32_t id_;
    std::string nickname_;
};

class ShowUserLeftCommand final : public IClientOutputCommand {
public:
    ShowUserLeftCommand(uint32_t id, std::string nickname)
        : id_(id), nickname_(std::move(nickname)) {}

    void execute(IClientOutputView& view) override { view.show_user_left(id_, nickname_); }

private:
    uint32_t id_;
    std::string nickname_;
};

class ShowUsersListCommand final : public IClientOutputCommand {
public:
    explicit ShowUsersListCommand(std::vector<protocol::UserInfo> users) : users_(std::move(users)) {}

    void execute(IClientOutputView& view) override { view.show_users_list(users_); }

private:
    std::vector<protocol::UserInfo> users_;
};

class ShowErrorCommand final : public IClientOutputCommand {
public:
    explicit ShowErrorCommand(std::string text) : text_(std::move(text)) {}

    void execute(IClientOutputView& view) override { view.show_error(text_); }

private:
    std::string text_;
};

class ShowNoticeCommand final : public IClientOutputCommand {
public:
    explicit ShowNoticeCommand(std::string text) : text_(std::move(text)) {}

    void execute(IClientOutputView& view) override { view.show_notice(text_); }

private:
    std::string text_;
};

class ShowSendFailedCommand final : public IClientOutputCommand {
public:
    void execute(IClientOutputView& view) override { view.show_send_failed(); }
};

class ShowInvalidFormatCommand final : public IClientOutputCommand {
public:
    void execute(IClientOutputView& view) override { view.show_invalid_format(); }
};

class ShowDecodeErrorCommand final : public IClientOutputCommand {
public:
    explicit ShowDecodeErrorCommand(std::string text) : text_(std::move(text)) {}

    void execute(IClientOutputView& view) override { view.show_decode_error(text_); }

private:
    std::string text_;
};

class ShowStatusCommand final : public IClientOutputCommand {
public:
    ShowStatusCommand(std::string step, int percent) : step_(std::move(step)), percent_(percent) {}

    void execute(IClientOutputView& view) override { view.show_status(step_, percent_); }

private:
    std::string step_;
    int percent_;
};

class ShowInputReadyCommand final : public IClientOutputCommand {
public:
    void execute(IClientOutputView& view) override { view.show_input_ready(); }
};

class ShowInputPromptCommand final : public IClientOutputCommand {
public:
    void execute(IClientOutputView& view) override { view.show_input_prompt(); }
};

class ShowExitingCommand final : public IClientOutputCommand {
public:
    void execute(IClientOutputView& view) override { view.show_exiting(); }
};

class ShowServerDisconnectedCommand final : public IClientOutputCommand {
public:
    void execute(IClientOutputView& view) override { view.show_server_disconnected(); }
};

}  // namespace client_view
