#include "viewmodel/ClientViewModel.hpp"

#include "Config.hpp"
#include "Protocol.hpp"
#include "StartupProgress.hpp"
#include "viewmodel/OutgoingMessageBuilder.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <string>

namespace {

std::string default_keystore_name(const std::string& nickname) {
    if (nickname.empty()) {
        return "client.keys";
    }
    return nickname + ".keys";
}

std::string default_log_name(const std::string& nickname) {
    if (nickname.empty()) {
        return config_defaults::kDefaultClientLogName;
    }
    return nickname + ".log";
}

}  // namespace

ClientViewModel::ClientViewModel(client_model::ClientSession session, IClientInputView& input_view,
                                 IClientOutputView& output_view, std::unique_ptr<IClientConnector> connector,
                                 std::unique_ptr<IMessageTransport> transport)
    : session_(std::move(session)),
      input_view_(input_view),
      output_view_(output_view),
      connector_(std::move(connector)),
      transport_(std::move(transport)),
      presenter_(output_view),
      keystore_file_(cipher_) {}

ClientViewModel::~ClientViewModel() {
    shutdown();
}

void ClientViewModel::request_shutdown(bool unblock_stdin) {
    running_ = false;
    const int fd = socket_fd_.load();
    if (fd >= 0) {
        ::shutdown(fd, SHUT_RDWR);
    }
    if (unblock_stdin) {
        ::close(STDIN_FILENO);
    }
}

void ClientViewModel::announce_exit(bool server_lost) {
    bool expected = false;
    if (!exit_announced_.compare_exchange_strong(expected, true)) {
        return;
    }
    if (server_lost) {
        output_view_.show_server_disconnected();
    } else {
        output_view_.show_exiting();
    }
}

void ClientViewModel::shutdown() {
    request_shutdown(false);
    if (receiver_.joinable()) {
        receiver_.join();
    }
    save_keystore();
    const int fd = session_.socket_fd();
    if (fd >= 0) {
        close(fd);
        session_.set_socket_fd(-1);
        socket_fd_.store(-1);
    }
}

std::string ClientViewModel::keystore_path() const {
    if (!session_.config().keystore_path.empty()) {
        return session_.config().keystore_path;
    }
    return default_keystore_name(session_.config().nickname);
}

std::string ClientViewModel::log_path() const {
    if (!session_.config().log_path.empty()) {
        return session_.config().log_path;
    }
    return default_log_name(session_.config().nickname);
}

void ClientViewModel::init_logger() {
    const std::string path = log_path();
    logger_.open(path);
    if (!logger_.is_open()) {
        output_view_.show_error("Cannot open log file: " + path);
        return;
    }
    logger_.info("client log started path=" + path);
    output_view_.show_notice("Log: " + path);
}

void ClientViewModel::report_error(const std::string& brief, const std::string& detail) {
    logger_.error(detail.empty() ? brief : (brief + " | " + detail));
    output_view_.show_error(brief);
}

void ClientViewModel::report_info(const std::string& brief, const std::string& detail) {
    logger_.info(detail.empty() ? brief : (brief + " | " + detail));
    output_view_.show_notice(brief);
}

std::string ClientViewModel::bytes_as_string(const std::vector<uint8_t>& bytes) const {
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

std::vector<uint8_t> ClientViewModel::string_as_bytes(const std::string& text) const {
    return std::vector<uint8_t>(text.begin(), text.end());
}

void ClientViewModel::load_or_create_identity() {
    std::lock_guard<std::mutex> lock(crypto_mutex_);
    const std::string path = keystore_path();
    const std::string& password = session_.config().keystore_password;
    try {
        if (keystore_file_.load(path, password, key_store_)) {
            report_info("Keystore loaded", "path=" + path);
            return;
        }
    } catch (const std::exception& ex) {
        report_error("Keystore load failed, creating new keys", ex.what());
    }

    auto pair = cipher_.generate_keypair();
    key_store_.set_identity(std::move(pair.secret_key), std::move(pair.public_key));
    report_info("Generated new identity keypair", "keystore=" + path);
}

void ClientViewModel::save_keystore() {
    std::lock_guard<std::mutex> lock(crypto_mutex_);
    if (!key_store_.has_identity()) {
        return;
    }
    try {
        keystore_file_.save(keystore_path(), session_.config().keystore_password, key_store_);
        logger_.info("Keystore saved path=" + keystore_path());
    } catch (const std::exception& ex) {
        report_error("Failed to save keystore", ex.what());
    }
}

void ClientViewModel::connect_to_server() {
    const auto& config = session_.config();
    output_view_.show_status("Connecting to " + config.server_ip + ":" + std::to_string(config.port),
                             client_startup::kConnectingPercent);
    const int socket_fd = connector_->connect(config);
    session_.set_socket_fd(socket_fd);
    socket_fd_.store(socket_fd);
    output_view_.show_status("TCP connected", client_startup::kTcpConnectedPercent);
    output_view_.show_connected(config.server_ip, config.port);
    logger_.info("connected to " + config.server_ip + ":" + std::to_string(config.port));
}

void ClientViewModel::receive_assigned_id() {
    output_view_.show_status("Waiting for assigned id", client_startup::kWaitAssignIdPercent);
    const auto message = transport_->receive(session_.socket_fd());
    if (!message || message->type != protocol::MsgType::AssignId) {
        throw std::runtime_error("Failed to receive assigned id");
    }

    const auto& assign = std::get<protocol::AssignIdPayload>(message->payload);
    session_.set_my_id(assign.id);
    output_view_.show_status("Got id=" + std::to_string(assign.id), client_startup::kGotIdPercent);
    output_view_.show_my_id(assign.id);
    logger_.info("assigned id=" + std::to_string(assign.id) +
                 " create_room=" + (assign.create_room ? "1" : "0"));

    if (assign.create_room) {
        std::lock_guard<std::mutex> lock(crypto_mutex_);
        if (!key_store_.has_room_key()) {
            key_store_.set_room_key(cipher_.generate_room_key());
            report_info("Created room key");
        }
    }
}

void ClientViewModel::register_nickname() {
    if (!session_.has_nickname()) {
        return;
    }
    output_view_.show_status("Registering nickname=" + session_.config().nickname,
                             client_startup::kRegisteringPercent);
    if (!transport_->send(session_.socket_fd(), protocol::MsgType::Register,
                          protocol::encode_register(session_.config().nickname))) {
        report_error("Failed to register nickname", "nickname=" + session_.config().nickname);
    }
}

void ClientViewModel::run() {
    init_logger();
    load_or_create_identity();
    connect_to_server();
    receive_assigned_id();
    register_nickname();
    output_view_.show_status("Startup complete", client_startup::kCompletePercent);
    output_view_.show_input_ready();
    output_view_.show_usage();
    receiver_ = std::thread(&ClientViewModel::receive_loop, this);
    input_loop();
    announce_exit(false);
    request_shutdown(false);
}

void ClientViewModel::input_loop() {
    while (running_) {
        output_view_.show_input_prompt();
        const auto line = input_view_.read_line();
        if (!line || line->empty()) {
            if (!line) {
                break;
            }
            continue;
        }

        handle_input_command(client_viewmodel::parse_input_line(*line, session_.my_id()));
        if (!running_) {
            break;
        }
    }
}

void ClientViewModel::send_key_offer(uint32_t peer_id) {
    if (peer_id == 0 || peer_id == session_.my_id()) {
        return;
    }
    std::string body;
    {
        std::lock_guard<std::mutex> lock(crypto_mutex_);
        if (key_offered_to_.count(peer_id) != 0) {
            return;
        }
        key_offered_to_.insert(peer_id);
        body = bytes_as_string(key_store_.identity_public());
    }
    if (!transport_->send(session_.socket_fd(), protocol::MsgType::KeyOffer,
                          client_viewmodel::OutgoingMessageBuilder::build_key_offer(session_.my_id(), peer_id,
                                                                                    body))) {
        std::lock_guard<std::mutex> lock(crypto_mutex_);
        key_offered_to_.erase(peer_id);
        report_error("Send failed", "KeyOffer to id=" + std::to_string(peer_id));
        return;
    }
    report_info("Sent KeyOffer", "peer_id=" + std::to_string(peer_id));
}

void ClientViewModel::maybe_send_room_key(uint32_t peer_id) {
    std::string body;
    {
        std::lock_guard<std::mutex> lock(crypto_mutex_);
        if (!key_store_.has_room_key()) {
            return;
        }
        const auto peer_pk = key_store_.peer_public(peer_id);
        if (!peer_pk) {
            return;
        }
        try {
            const auto sealed =
                cipher_.seal_room_key(key_store_.identity_secret(), *peer_pk, *key_store_.room_key());
            body = bytes_as_string(sealed);
        } catch (const std::exception& ex) {
            report_error("Failed to seal room key",
                         "peer_id=" + std::to_string(peer_id) + " err=" + ex.what());
            return;
        }
    }
    if (!transport_->send(session_.socket_fd(), protocol::MsgType::RoomKeyOffer,
                          client_viewmodel::OutgoingMessageBuilder::build_room_key_offer(
                              session_.my_id(), peer_id, body))) {
        report_error("Send failed", "RoomKeyOffer to id=" + std::to_string(peer_id));
        return;
    }
    report_info("Sent RoomKeyOffer", "peer_id=" + std::to_string(peer_id));
}

void ClientViewModel::send_encrypted_chat_by_id(uint32_t recipient_id, const std::string& plaintext) {
    std::string cipher_body;
    {
        std::lock_guard<std::mutex> lock(crypto_mutex_);
        const auto peer_pk = key_store_.peer_public(recipient_id);
        if (!peer_pk) {
            report_error("No key for peer",
                         "id=" + std::to_string(recipient_id) + " hint=/key " +
                             std::to_string(recipient_id));
            return;
        }
        try {
            const auto sealed =
                cipher_.encrypt_for_peer(key_store_.identity_secret(), *peer_pk, plaintext);
            cipher_body = bytes_as_string(sealed);
        } catch (const std::exception& ex) {
            report_error("Encrypt failed", "id=" + std::to_string(recipient_id) + " err=" + ex.what());
            return;
        }
    }
    if (!transport_->send(session_.socket_fd(), protocol::MsgType::Chat,
                          client_viewmodel::OutgoingMessageBuilder::build_chat_by_id(
                              session_.my_id(), recipient_id, cipher_body))) {
        report_error("Send failed", "Chat to id=" + std::to_string(recipient_id));
    }
}

void ClientViewModel::send_encrypted_chat_by_nickname(const std::string& nickname,
                                                      const std::string& plaintext) {
    uint32_t recipient_id = 0;
    {
        std::lock_guard<std::mutex> lock(crypto_mutex_);
        const auto it = id_by_nickname_.find(nickname);
        if (it == id_by_nickname_.end()) {
            report_error("Unknown nickname", "nickname=" + nickname + " hint=/users");
            return;
        }
        recipient_id = it->second;
    }
    send_encrypted_chat_by_id(recipient_id, plaintext);
}

void ClientViewModel::send_encrypted_broadcast(const std::string& plaintext) {
    std::string cipher_body;
    {
        std::lock_guard<std::mutex> lock(crypto_mutex_);
        if (!key_store_.has_room_key()) {
            report_error("No room key yet", "wait for RoomKeyOffer or be first client");
            return;
        }
        try {
            const auto sealed = cipher_.encrypt_with_room_key(*key_store_.room_key(), plaintext);
            cipher_body = bytes_as_string(sealed);
        } catch (const std::exception& ex) {
            report_error("Encrypt broadcast failed", ex.what());
            return;
        }
    }
    if (!transport_->send(session_.socket_fd(), protocol::MsgType::Chat,
                          client_viewmodel::OutgoingMessageBuilder::build_chat_broadcast(
                              session_.my_id(), cipher_body))) {
        report_error("Send failed", "Chat broadcast");
    }
}

std::optional<std::string> ClientViewModel::decrypt_deliver(uint32_t from_id, const std::string& body) {
    std::lock_guard<std::mutex> lock(crypto_mutex_);
    const auto cipher_bytes = string_as_bytes(body);
    std::string peer_err;
    std::string room_err;

    if (const auto peer_pk = key_store_.peer_public(from_id)) {
        try {
            return cipher_.decrypt_from_peer(key_store_.identity_secret(), *peer_pk, cipher_bytes);
        } catch (const std::exception& ex) {
            peer_err = ex.what();
        }
    } else {
        peer_err = "no peer key";
    }

    if (key_store_.has_room_key()) {
        try {
            return cipher_.decrypt_with_room_key(*key_store_.room_key(), cipher_bytes);
        } catch (const std::exception& ex) {
            room_err = ex.what();
        }
    } else {
        room_err = "no room key";
    }

    logger_.error("decrypt failed from_id=" + std::to_string(from_id) + " peer=[" + peer_err +
                  "] room=[" + room_err + "]");
    return std::nullopt;
}

void ClientViewModel::handle_input_command(const client_viewmodel::InputCommand& command) {
    switch (command.type) {
        case client_viewmodel::InputCommandType::ListUsers:
            if (!transport_->send(session_.socket_fd(), protocol::MsgType::ListUsers,
                                  client_viewmodel::OutgoingMessageBuilder::build_list_users())) {
                report_error("Send failed", "ListUsers");
            }
            break;
        case client_viewmodel::InputCommandType::Exit:
            running_ = false;
            break;
        case client_viewmodel::InputCommandType::KeyOffer:
            send_key_offer(command.recipient_id);
            break;
        case client_viewmodel::InputCommandType::ChatById:
            send_encrypted_chat_by_id(command.recipient_id, command.text);
            break;
        case client_viewmodel::InputCommandType::ChatByNickname:
            send_encrypted_chat_by_nickname(command.recipient_nickname, command.text);
            break;
        case client_viewmodel::InputCommandType::ChatBroadcast:
            send_encrypted_broadcast(command.text);
            break;
        case client_viewmodel::InputCommandType::Invalid:
            report_error("Invalid format", "use recipient:text or /users /key /exit");
            break;
    }
}

void ClientViewModel::handle_incoming(const protocol::AppMessage& message) {
    switch (message.type) {
        case protocol::MsgType::Deliver: {
            const auto& deliver = std::get<protocol::DeliverPayload>(message.payload);
            const auto plain = decrypt_deliver(deliver.from_id, deliver.text);
            if (!plain) {
                report_error("Decrypt failed", "from_id=" + std::to_string(deliver.from_id));
                return;
            }
            output_view_.show_delivered(deliver.from_id, *plain);
            break;
        }
        case protocol::MsgType::PeerKey: {
            const auto& deliver = std::get<protocol::DeliverPayload>(message.payload);
            try {
                const auto pk = string_as_bytes(deliver.text);
                {
                    std::lock_guard<std::mutex> lock(crypto_mutex_);
                    key_store_.set_peer_public(deliver.from_id, pk);
                }
                report_info("Received pubkey", "from_id=" + std::to_string(deliver.from_id));
                send_key_offer(deliver.from_id);
                maybe_send_room_key(deliver.from_id);
            } catch (const std::exception& ex) {
                report_error("Invalid PeerKey",
                             "from_id=" + std::to_string(deliver.from_id) + " err=" + ex.what());
            }
            break;
        }
        case protocol::MsgType::RoomKey: {
            const auto& deliver = std::get<protocol::DeliverPayload>(message.payload);
            try {
                std::lock_guard<std::mutex> lock(crypto_mutex_);
                const auto peer_pk = key_store_.peer_public(deliver.from_id);
                if (!peer_pk) {
                    report_error("RoomKey from unknown peer",
                                 "from_id=" + std::to_string(deliver.from_id));
                    return;
                }
                auto room = cipher_.open_room_key(key_store_.identity_secret(), *peer_pk,
                                                  string_as_bytes(deliver.text));
                key_store_.set_room_key(std::move(room));
                report_info("Received room key", "from_id=" + std::to_string(deliver.from_id));
            } catch (const std::exception& ex) {
                report_error("Failed to open room key", ex.what());
            }
            break;
        }
        case protocol::MsgType::BannedIds: {
            const auto& banned = std::get<protocol::BannedIdsPayload>(message.payload);
            {
                std::lock_guard<std::mutex> lock(crypto_mutex_);
                key_store_.erase_peers(banned.ids);
            }
            std::string ids;
            for (size_t i = 0; i < banned.ids.size(); ++i) {
                if (i > 0) {
                    ids += ',';
                }
                ids += std::to_string(banned.ids[i]);
            }
            report_info("Admin banned ids", ids);
            break;
        }
        case protocol::MsgType::UserJoined: {
            const auto& joined = std::get<protocol::UserJoinedPayload>(message.payload);
            {
                std::lock_guard<std::mutex> lock(crypto_mutex_);
                nickname_by_id_[joined.id] = joined.nickname;
                if (!joined.nickname.empty()) {
                    id_by_nickname_[joined.nickname] = joined.id;
                }
            }
            presenter_.present(message);
            if (joined.id != session_.my_id()) {
                send_key_offer(joined.id);
            }
            break;
        }
        case protocol::MsgType::UserLeft: {
            const auto& left = std::get<protocol::UserLeftPayload>(message.payload);
            {
                std::lock_guard<std::mutex> lock(crypto_mutex_);
                nickname_by_id_.erase(left.id);
                if (!left.nickname.empty()) {
                    id_by_nickname_.erase(left.nickname);
                }
            }
            presenter_.present(message);
            break;
        }
        case protocol::MsgType::UsersList: {
            const auto& list = std::get<protocol::UsersListPayload>(message.payload);
            {
                std::lock_guard<std::mutex> lock(crypto_mutex_);
                for (const auto& user : list.users) {
                    nickname_by_id_[user.id] = user.nickname;
                    if (!user.nickname.empty()) {
                        id_by_nickname_[user.nickname] = user.id;
                    }
                }
            }
            presenter_.present(message);
            break;
        }
        default:
            presenter_.present(message);
            break;
    }
}

void ClientViewModel::receive_loop() {
    while (running_) {
        try {
            const auto message = transport_->receive(session_.socket_fd());
            if (!message) {
                announce_exit(true);
                request_shutdown(/*unblock_stdin=*/true);
                break;
            }
            handle_incoming(*message);
        } catch (const std::exception& ex) {
            report_error("Decode failed", ex.what());
        }
    }
}
