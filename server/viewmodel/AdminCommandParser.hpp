#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace server_admin {

struct BanCommand {
    std::vector<uint32_t> ids;
};

// Parses "ban:1,2,3" (spaces allowed). Returns nullopt on invalid input.
std::optional<BanCommand> parse_admin_command(const std::string& line);

std::string admin_help_text();

}  // namespace server_admin
