#include "view/ConsoleClientView.hpp"
#include "view/QueuedClientOutputView.hpp"
#include "viewmodel/ClientViewModel.hpp"

#include "Config.hpp"
#include "StartupProgress.hpp"
#include "model/ClientSession.hpp"
#include "transport/ModbusMessageTransport.hpp"
#include "transport/TcpClientConnector.hpp"

#include <csignal>
#include <future>
#include <iostream>
#include <memory>
#include <string>

namespace {

ClientViewModel* g_client_vm = nullptr;

void on_client_signal(int) {
    if (g_client_vm != nullptr) {
        g_client_vm->request_shutdown(/*unblock_stdin=*/true);
    }
}

void install_client_signal_handlers() {
    struct sigaction action {};
    action.sa_handler = on_client_signal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGHUP, &action, nullptr);
}

ClientConfig load_client_config_async(const char* argv0, const char* override_path,
                                      IClientOutputView& output) {
    auto future = std::async(std::launch::async, [&output, argv0, override_path]() {
        output.show_status("Resolving config path", client_startup::kResolveConfigPercent);
        const std::string config_path = Config::resolve_config_path(
            argv0, config_defaults::kDefaultClientConfigName, override_path);

        output.show_status("Reading " + config_path, client_startup::kReadConfigBeginPercent);
        return ClientConfig::from_file(config_path, [&output](int file_percent, const std::string& msg) {
            output.show_status(msg, client_startup::map_config_file_percent(file_percent));
        });
    });

    return future.get();
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        ConsoleClientView console;
        QueuedClientOutputView output(console);

        ClientConfig config =
            load_client_config_async(argv[0], argc > 1 ? argv[1] : nullptr, output);
        output.show_status("Config ready", client_startup::kConfigReadyPercent);

        ClientViewModel view_model(client_model::ClientSession(std::move(config)), console, output,
                                   std::make_unique<TcpClientConnector>(),
                                   std::make_unique<ModbusMessageTransport>());
        g_client_vm = &view_model;
        install_client_signal_handlers();
        view_model.run();
        g_client_vm = nullptr;
        output.stop();
    } catch (const std::exception& ex) {
        g_client_vm = nullptr;
        std::cerr << "Client error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
