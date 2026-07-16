#pragma once

#include <cstdint>
#include <string>

namespace server_model {

struct ClientRecord {
    int fd{-1};
    uint32_t id{0};
    std::string nickname;
    bool registered{false};
};

}  // namespace server_model
