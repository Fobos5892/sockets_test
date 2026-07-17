#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "crypto/EncryptedKeyStoreFile.hpp"
#include "crypto/IMessageCipher.hpp"
#include "crypto/MonocypherMessageCipher.hpp"
#include "crypto/PeerKeyStore.hpp"
#include "logging/FileLogger.hpp"
#include "model/ClientSession.hpp"
#include "transport/IClientConnector.hpp"
#include "transport/IMessageTransport.hpp"
#include "view/IClientInputView.hpp"
#include "view/IClientOutputView.hpp"
#include "viewmodel/IncomingMessagePresenter.hpp"
#include "viewmodel/InputCommandParser.hpp"

class ClientViewModel {
public:
    ClientViewModel(client_model::ClientSession session, IClientInputView& input_view,
                    IClientOutputView& output_view, std::unique_ptr<IClientConnector> connector,
                    std::unique_ptr<IMessageTransport> transport);
    ~ClientViewModel();

    void run();

    // Safe to call from a signal handler when unblock_stdin is true (closes stdin to wake getline).
    void request_shutdown(bool unblock_stdin = false);

private:
    void connect_to_server();
    void receive_assigned_id();
    void register_nickname();
    void input_loop();
    void receive_loop();
    void handle_input_command(const client_viewmodel::InputCommand& command);
    void handle_incoming(const protocol::AppMessage& message);
    void announce_exit(bool server_lost);
    void shutdown();

    void init_logger();
    void report_error(const std::string& brief, const std::string& detail);
    void report_info(const std::string& brief, const std::string& detail = {});

    void load_or_create_identity();
    void save_keystore();
    std::string keystore_path() const;
    std::string log_path() const;
    std::string bytes_as_string(const std::vector<uint8_t>& bytes) const;
    std::vector<uint8_t> string_as_bytes(const std::string& text) const;

    void send_key_offer(uint32_t peer_id);
    void maybe_send_room_key(uint32_t peer_id);
    void send_encrypted_chat_by_id(uint32_t recipient_id, const std::string& plaintext);
    void send_encrypted_chat_by_nickname(const std::string& nickname, const std::string& plaintext);
    void send_encrypted_broadcast(const std::string& plaintext);
    std::optional<std::string> decrypt_deliver(uint32_t from_id, const std::string& body);

    client_model::ClientSession session_;
    IClientInputView& input_view_;
    IClientOutputView& output_view_;
    std::unique_ptr<IClientConnector> connector_;
    std::unique_ptr<IMessageTransport> transport_;
    IncomingMessagePresenter presenter_;
    logging::FileLogger logger_;

    crypto::MonocypherMessageCipher cipher_;
    crypto::PeerKeyStore key_store_;
    crypto::EncryptedKeyStoreFile keystore_file_;
    std::mutex crypto_mutex_;
    std::unordered_map<std::string, uint32_t> id_by_nickname_;
    std::unordered_map<uint32_t, std::string> nickname_by_id_;
    std::unordered_set<uint32_t> key_offered_to_;

    std::atomic<bool> running_{true};
    std::atomic<bool> exit_announced_{false};
    std::atomic<int> socket_fd_{-1};
    std::thread receiver_;
};
