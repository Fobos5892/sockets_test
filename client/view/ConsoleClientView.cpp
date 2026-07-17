#include "view/ConsoleClientView.hpp"

#include "StartupProgress.hpp"

#include <iostream>

void ConsoleClientView::show_status(const std::string& step, int percent) {
    const int clamped = percent < client_startup::kProgressMin
                            ? client_startup::kProgressMin
                            : (percent > client_startup::kProgressMax ? client_startup::kProgressMax
                                                                       : percent);
    std::cout << "[startup " << clamped << "%] " << step << std::endl;
}

void ConsoleClientView::show_input_ready() {
    std::cout << "\n"
              << "============================================================\n"
              << "  CHAT READY — type your message below\n"
              << "------------------------------------------------------------\n"
              << "  Format:   <who>:<text>\n"
              << "  who   =   nickname  or  numeric id\n"
              << "  text  =   message body\n"
              << "------------------------------------------------------------\n"
              << "  Examples:  bob:hello\n"
              << "             2:hello\n"
              << "             all:hello\n"
              << "  Commands:  /users   /key <id>   /exit\n"
              << "============================================================\n"
              << std::flush;
}

void ConsoleClientView::show_input_prompt() {
    // Permanent hint on every turn (incoming chat scrolls the banner away).
    std::cout << "\n[INPUT] who:message  (example: bob:hello | all:hello)  |  /users  |  /key id  |  /exit\n"
              << ">>> " << std::flush;
}

void ConsoleClientView::show_usage() {
    // Kept for API; ready banner already explains the format.
}

void ConsoleClientView::show_connected(const std::string& server_ip, uint16_t port) {
    std::cout << "Connected to " << server_ip << ':' << port << std::endl;
}

void ConsoleClientView::show_my_id(uint32_t id) {
    std::cout << "My id=" << id << std::endl;
}

std::optional<std::string> ConsoleClientView::read_line() {
    std::string line;
    if (!std::getline(std::cin, line)) {
        return std::nullopt;
    }
    return line;
}

void ConsoleClientView::show_delivered(uint32_t from_id, const std::string& text) {
    std::cout << "[from " << from_id << "] " << text << std::endl;
}

void ConsoleClientView::show_user_joined(uint32_t id, const std::string& nickname) {
    std::cout << "[presence] User joined: id=" << id;
    if (!nickname.empty()) {
        std::cout << " nickname=" << nickname;
    }
    std::cout << std::endl;
}

void ConsoleClientView::show_user_left(uint32_t id, const std::string& nickname) {
    std::cout << "[presence] User left: id=" << id;
    if (!nickname.empty()) {
        std::cout << " nickname=" << nickname;
    }
    std::cout << std::endl;
}

void ConsoleClientView::show_users_list(const std::vector<protocol::UserInfo>& users) {
    std::cout << "Connected users (" << users.size() << "):" << std::endl;
    for (const auto& user : users) {
        std::cout << "  id=" << user.id;
        if (!user.nickname.empty()) {
            std::cout << " nickname=" << user.nickname;
        }
        std::cout << std::endl;
    }
}

void ConsoleClientView::show_error(const std::string& text) {
    std::cerr << "Server error: " << text << std::endl;
}

void ConsoleClientView::show_notice(const std::string& text) {
    std::cout << "[notice] " << text << std::endl;
}

void ConsoleClientView::show_send_failed() {
    std::cerr << "Failed to send message" << std::endl;
}

void ConsoleClientView::show_invalid_format() {
    std::cerr << "Invalid format, use recipient:text" << std::endl;
}

void ConsoleClientView::show_decode_error(const std::string& text) {
    std::cerr << "Failed to decode message: " << text << std::endl;
}

void ConsoleClientView::show_exiting() {
    std::cout << "Leaving chat — notifying others and closing connection..." << std::endl;
}

void ConsoleClientView::show_server_disconnected() {
    std::cout << "Server closed the connection — exiting..." << std::endl;
}
