#include "view/QueuedClientOutputView.hpp"

QueuedClientOutputView::QueuedClientOutputView(IClientOutputView& target) : executor_(target) {}

void QueuedClientOutputView::post(std::unique_ptr<IClientOutputCommand> command) {
    executor_.post(std::move(command));
}

void QueuedClientOutputView::stop() {
    executor_.stop();
}

void QueuedClientOutputView::show_status(const std::string& step, int percent) {
    post(std::make_unique<client_view::ShowStatusCommand>(step, percent));
}

void QueuedClientOutputView::show_input_ready() {
    post(std::make_unique<client_view::ShowInputReadyCommand>());
}

void QueuedClientOutputView::show_input_prompt() {
    post(std::make_unique<client_view::ShowInputPromptCommand>());
}

void QueuedClientOutputView::show_connected(const std::string& server_ip, uint16_t port) {
    post(std::make_unique<client_view::ShowConnectedCommand>(server_ip, port));
}

void QueuedClientOutputView::show_my_id(uint32_t id) {
    post(std::make_unique<client_view::ShowMyIdCommand>(id));
}

void QueuedClientOutputView::show_usage() {
    post(std::make_unique<client_view::ShowUsageCommand>());
}

void QueuedClientOutputView::show_delivered(uint32_t from_id, const std::string& text) {
    post(std::make_unique<client_view::ShowDeliveredCommand>(from_id, text));
}

void QueuedClientOutputView::show_user_joined(uint32_t id, const std::string& nickname) {
    post(std::make_unique<client_view::ShowUserJoinedCommand>(id, nickname));
}

void QueuedClientOutputView::show_user_left(uint32_t id, const std::string& nickname) {
    post(std::make_unique<client_view::ShowUserLeftCommand>(id, nickname));
}

void QueuedClientOutputView::show_users_list(const std::vector<protocol::UserInfo>& users) {
    post(std::make_unique<client_view::ShowUsersListCommand>(users));
}

void QueuedClientOutputView::show_error(const std::string& text) {
    post(std::make_unique<client_view::ShowErrorCommand>(text));
}

void QueuedClientOutputView::show_notice(const std::string& text) {
    post(std::make_unique<client_view::ShowNoticeCommand>(text));
}

void QueuedClientOutputView::show_send_failed() {
    post(std::make_unique<client_view::ShowSendFailedCommand>());
}

void QueuedClientOutputView::show_invalid_format() {
    post(std::make_unique<client_view::ShowInvalidFormatCommand>());
}

void QueuedClientOutputView::show_decode_error(const std::string& text) {
    post(std::make_unique<client_view::ShowDecodeErrorCommand>(text));
}

void QueuedClientOutputView::show_exiting() {
    post(std::make_unique<client_view::ShowExitingCommand>());
}

void QueuedClientOutputView::show_server_disconnected() {
    post(std::make_unique<client_view::ShowServerDisconnectedCommand>());
}
