#include "view/ConsoleClientView.hpp"
#include "view/QueuedClientOutputView.hpp"
#include "viewmodel/ClientViewModel.hpp"

#include "Config.hpp"
#include "model/ClientSession.hpp"
#include "transport/ModbusMessageTransport.hpp"
#include "transport/TcpClientConnector.hpp"

#include <future>
#include <iostream>
#include <memory>
#include <string>

namespace {

ClientConfig load_client_config_async(const char* argv0, const char* override_path,
                                      IClientOutputView& output) {
    auto future = std::async(std::launch::async, [&output, argv0, override_path]() {
        output.show_status("Resolving config path", 5);
        const std::string config_path =
            Config::resolve_config_path(argv0, "client.conf", override_path);

        output.show_status("Reading " + config_path, 10);
        return ClientConfig::from_file(config_path, [&output](int file_percent, const std::string& msg) {
            // Map file read 0..100% into overall startup band 10..35%.
            const int overall = 10 + (file_percent * 25) / 100;
            output.show_status(msg, overall);
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
        output.show_status("Config ready", 40);

        ClientViewModel view_model(client_model::ClientSession(std::move(config)), console, output,
                                   std::make_unique<TcpClientConnector>(),
                                   std::make_unique<ModbusMessageTransport>());
        view_model.run();
        output.stop();
    } catch (const std::exception& ex) {
        std::cerr << "Client error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
