#pragma once

#include "Config.hpp"
#include "view/IServerView.hpp"
#include "view/ServerLogView.hpp"
#include "viewmodel/ServerViewModel.hpp"

#include <cstdint>
#include <string>
#include <vector>

class Server {
public:
    explicit Server(ServerConfig config);

    void run();
    void request_stop();
    bool is_listening() const;
    uint16_t port() const;

    void submit_admin_line(std::string line);
    void ban_ids(const std::vector<uint32_t>& ids);

private:
    ServerLogView view_;
    ServerViewModel view_model_;
};
