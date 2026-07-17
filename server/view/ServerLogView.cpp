#include "view/ServerLogView.hpp"

#include <iostream>

void ServerLogView::show_listening(const std::string& bind_ip, uint16_t port) {
    std::cout << "Server listening on " << bind_ip << ':' << port << std::endl;
    std::cout << "Admin: " << "ban:1,2,3" << std::endl;
}

void ServerLogView::show_accept_failed(const std::string& error) {
    std::cerr << "accept failed: " << error << std::endl;
}

void ServerLogView::show_client_connected(uint32_t id) {
    std::cout << "Client connected, assigned id=" << id << std::endl;
}

void ServerLogView::show_client_disconnected(uint32_t id, const std::string& nickname) {
    std::cout << "Client disconnected id=" << id;
    if (!nickname.empty()) {
        std::cout << " nickname=" << nickname;
    }
    std::cout << std::endl;
}

void ServerLogView::show_client_registered(uint32_t id, const std::string& nickname) {
    std::cout << "Client id=" << id << " registered nickname=" << nickname << std::endl;
}

void ServerLogView::show_chat(uint32_t from_id, const std::string& to_label, const std::string& kind) {
    std::cout << kind << " from id=" << from_id << " to " << to_label << std::endl;
}

void ServerLogView::show_decode_error(const std::string& error) {
    std::cerr << "Failed to decode message: " << error << std::endl;
}

void ServerLogView::show_admin_message(const std::string& text) {
    std::cout << "[admin] " << text << std::endl;
}
