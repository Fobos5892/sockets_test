#pragma once

#include <memory>

#include "concurrency/QueuedCommandExecutor.hpp"
#include "view/ClientOutputCommands.hpp"
#include "view/IClientOutputCommand.hpp"
#include "view/IClientOutputView.hpp"

// Adapter: IClientOutputView API -> Command objects -> shared QueuedCommandExecutor.
class QueuedClientOutputView final : public IClientOutputView {
public:
    explicit QueuedClientOutputView(IClientOutputView& target);
    ~QueuedClientOutputView() override = default;

    QueuedClientOutputView(const QueuedClientOutputView&) = delete;
    QueuedClientOutputView& operator=(const QueuedClientOutputView&) = delete;

    void show_status(const std::string& step, int percent) override;
    void show_input_ready() override;
    void show_input_prompt() override;
    void show_connected(const std::string& server_ip, uint16_t port) override;
    void show_my_id(uint32_t id) override;
    void show_usage() override;
    void show_delivered(uint32_t from_id, const std::string& text) override;
    void show_user_joined(uint32_t id, const std::string& nickname) override;
    void show_user_left(uint32_t id, const std::string& nickname) override;
    void show_users_list(const std::vector<protocol::UserInfo>& users) override;
    void show_error(const std::string& text) override;
    void show_notice(const std::string& text) override;
    void show_send_failed() override;
    void show_invalid_format() override;
    void show_decode_error(const std::string& text) override;
    void show_exiting() override;
    void show_server_disconnected() override;

    void post(std::unique_ptr<IClientOutputCommand> command);
    void stop();

private:
    QueuedCommandExecutor<IClientOutputView> executor_;
};
