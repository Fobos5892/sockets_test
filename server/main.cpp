#include "Server.hpp"

#include "Config.hpp"

#include <csignal>
#include <iostream>
#include <string>
#include <thread>

namespace {

Server* g_server = nullptr;

void on_server_signal(int) {
    if (g_server != nullptr) {
        g_server->request_stop();
    }
}

void install_server_signal_handlers() {
    struct sigaction action {};
    action.sa_handler = on_server_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGHUP, &action, nullptr);
}

void admin_input_loop() {
    std::string line;
    while (g_server != nullptr && std::getline(std::cin, line)) {
        if (g_server == nullptr) {
            break;
        }
        g_server->submit_admin_line(std::move(line));
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    // Broken peer sockets must not kill the process during broadcast/send.
    std::signal(SIGPIPE, SIG_IGN);

    const std::string config_path =
        Config::resolve_config_path(argv[0], config_defaults::kDefaultServerConfigName,
                                    argc > 1 ? argv[1] : nullptr);

    try {
        ServerConfig config = ServerConfig::from_file(config_path);
        Server server(std::move(config));
        g_server = &server;
        install_server_signal_handlers();
        std::thread admin_thread(admin_input_loop);
        server.run();
        g_server = nullptr;
        // Unblock admin getline if still waiting.
        std::cin.setstate(std::ios::eofbit);
        if (admin_thread.joinable()) {
            admin_thread.detach();
        }
    } catch (const std::exception& ex) {
        g_server = nullptr;
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
