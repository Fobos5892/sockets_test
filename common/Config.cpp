#include "Config.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

Config Config::load(const std::string& path) {
    Config config;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + path);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) {
            key.pop_back();
        }
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.erase(value.begin());
        }

        config.values_[key] = value;
    }

    return config;
}

std::string Config::get(const std::string& key, const std::string& default_value) const {
    const auto it = values_.find(key);
    if (it == values_.end()) {
        return default_value;
    }
    return it->second;
}

ServerConfig ServerConfig::from_file(const std::string& path) {
    const Config config = Config::load(path);
    ServerConfig server;
    server.bind_ip = config.get("bind_ip", "0.0.0.0");
    server.port = static_cast<uint16_t>(std::stoi(config.get("port", "5020")));
    return server;
}

ClientConfig ClientConfig::from_file(const std::string& path) {
    const Config config = Config::load(path);
    ClientConfig client;
    client.server_ip = config.get("server_ip", "127.0.0.1");
    client.port = static_cast<uint16_t>(std::stoi(config.get("port", "5020")));
    client.nickname = config.get("nickname", "");
    return client;
}
