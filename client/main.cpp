#include "view/ConsoleClientView.hpp"
#include "viewmodel/ClientViewModel.hpp"

#include "Config.hpp"
#include "model/ClientSession.hpp"
#include "transport/ModbusMessageTransport.hpp"
#include "transport/TcpClientConnector.hpp"

#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    const std::string config_path = (argc > 1) ? argv[1] : "config/client.conf";

    try {
        ClientConfig config = ClientConfig::from_file(config_path);
        ConsoleClientView view;
        ClientViewModel view_model(client_model::ClientSession(std::move(config)), view, view,
                                   std::make_unique<TcpClientConnector>(),
                                   std::make_unique<ModbusMessageTransport>());
        view_model.run();
    } catch (const std::exception& ex) {
        std::cerr << "Client error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
