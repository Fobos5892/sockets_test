#pragma once

#include "Config.hpp"
#include "view/IServerView.hpp"
#include "view/ServerLogView.hpp"
#include "viewmodel/ServerViewModel.hpp"

class Server {
public:
    explicit Server(ServerConfig config);

    void run();
    void request_stop();
    bool is_listening() const;
    uint16_t port() const;

private:
    ServerLogView view_;
    ServerViewModel view_model_;
};
