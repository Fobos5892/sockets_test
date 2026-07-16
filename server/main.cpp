#include "Server.hpp"

#include "Config.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    const std::string config_path =
        Config::resolve_config_path(argv[0], "server.conf", argc > 1 ? argv[1] : nullptr);

    try {
        ServerConfig config = ServerConfig::from_file(config_path);
        Server server(std::move(config));
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
