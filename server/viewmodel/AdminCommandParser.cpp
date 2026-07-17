#include "viewmodel/AdminCommandParser.hpp"

#include <cctype>
#include <sstream>

namespace server_admin {

namespace {

std::string trim(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

}  // namespace

std::optional<BanCommand> parse_admin_command(const std::string& line) {
    const std::string trimmed = trim(line);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    constexpr const char* kBanPrefix = "ban:";
    constexpr size_t kBanPrefixLen = 4;
    if (trimmed.size() < kBanPrefixLen ||
        trimmed.compare(0, kBanPrefixLen, kBanPrefix) != 0) {
        return std::nullopt;
    }

    BanCommand command;
    std::string ids_part = trim(trimmed.substr(kBanPrefixLen));
    if (ids_part.empty()) {
        return std::nullopt;
    }

    std::stringstream stream(ids_part);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = trim(token);
        if (token.empty()) {
            return std::nullopt;
        }
        try {
            size_t consumed = 0;
            const unsigned long value = std::stoul(token, &consumed, 10);
            if (consumed != token.size() || value == 0 || value > 0xFFFFFFFFul) {
                return std::nullopt;
            }
            command.ids.push_back(static_cast<uint32_t>(value));
        } catch (...) {
            return std::nullopt;
        }
    }

    if (command.ids.empty()) {
        return std::nullopt;
    }
    return command;
}

std::string admin_help_text() {
    return "Admin commands: ban:1,2,3";
}

}  // namespace server_admin
