#include "viewmodel/ClientViewModel.hpp"

#include "Protocol.hpp"
#include "viewmodel/OutgoingMessageBuilder.hpp"

#include <sys/socket.h>
#include <unistd.h>

ClientViewModel::ClientViewModel(client_model::ClientSession session, IClientInputView& input_view,
                                 IClientOutputView& output_view, std::unique_ptr<IClientConnector> connector,
                                 std::unique_ptr<IMessageTransport> transport)
    : session_(std::move(session)),
      input_view_(input_view),
      output_view_(output_view),
      connector_(std::move(connector)),
      transport_(std::move(transport)),
      presenter_(output_view) {}

ClientViewModel::~ClientViewModel() {
    shutdown();
}

void ClientViewModel::shutdown() {
    running_ = false;
    if (session_.socket_fd() >= 0) {
        ::shutdown(session_.socket_fd(), SHUT_RDWR);
    }
    if (receiver_.joinable()) {
        receiver_.join();
    }
    if (session_.socket_fd() >= 0) {
        close(session_.socket_fd());
        session_.set_socket_fd(-1);
    }
}

void ClientViewModel::connect_to_server() {
    const auto& config = session_.config();
    const int socket_fd = connector_->connect(config);
    session_.set_socket_fd(socket_fd);
    output_view_.show_connected(config.server_ip, config.port);
}

void ClientViewModel::receive_assigned_id() {
    const auto message = transport_->receive(session_.socket_fd());
    if (!message || message->type != protocol::MsgType::AssignId) {
        throw std::runtime_error("Failed to receive assigned id");
    }

    const uint32_t my_id = std::get<protocol::AssignIdPayload>(message->payload).id;
    session_.set_my_id(my_id);
    output_view_.show_my_id(my_id);
}

void ClientViewModel::register_nickname() {
    if (!session_.has_nickname()) {
        return;
    }
    transport_->send(session_.socket_fd(), protocol::MsgType::Register,
                     protocol::encode_register(session_.config().nickname));
}

void ClientViewModel::run() {
    connect_to_server();
    receive_assigned_id();
    register_nickname();
    receiver_ = std::thread(&ClientViewModel::receive_loop, this);
    input_loop();
}

void ClientViewModel::input_loop() {
    output_view_.show_usage();

    while (running_) {
        const auto line = input_view_.read_line();
        if (!line || line->empty()) {
            if (!line) {
                break;
            }
            continue;
        }

        handle_input_command(client_viewmodel::parse_input_line(*line, session_.my_id()));
    }
}

void ClientViewModel::handle_input_command(const client_viewmodel::InputCommand& command) {
    switch (command.type) {
        case client_viewmodel::InputCommandType::ListUsers:
            if (!transport_->send(session_.socket_fd(), protocol::MsgType::ListUsers,
                                  client_viewmodel::OutgoingMessageBuilder::build_list_users())) {
                output_view_.show_send_failed();
            }
            break;
        case client_viewmodel::InputCommandType::ChatById:
            if (!transport_->send(session_.socket_fd(), protocol::MsgType::Chat,
                                   client_viewmodel::OutgoingMessageBuilder::build_chat_by_id(
                                       session_.my_id(), command.recipient_id, command.text))) {
                output_view_.show_send_failed();
            }
            break;
        case client_viewmodel::InputCommandType::ChatByNickname:
            if (!transport_->send(session_.socket_fd(), protocol::MsgType::Chat,
                                   client_viewmodel::OutgoingMessageBuilder::build_chat_by_nickname(
                                       session_.my_id(), command.recipient_nickname, command.text))) {
                output_view_.show_send_failed();
            }
            break;
        case client_viewmodel::InputCommandType::Invalid:
            output_view_.show_invalid_format();
            break;
    }
}

void ClientViewModel::receive_loop() {
    while (running_) {
        try {
            const auto message = transport_->receive(session_.socket_fd());
            if (!message) {
                running_ = false;
                break;
            }
            presenter_.present(*message);
        } catch (const std::exception& ex) {
            output_view_.show_decode_error(ex.what());
        }
    }
}
