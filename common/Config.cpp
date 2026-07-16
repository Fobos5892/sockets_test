#include "Config.hpp"

#include <climits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace {

std::string executable_dir(const char* argv0) {
    char buf[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        std::string path(buf);
        const auto pos = path.find_last_of('/');
        if (pos != std::string::npos) {
            return path.substr(0, pos);
        }
    }

    if (argv0 != nullptr && argv0[0] != '\0') {
        std::string path(argv0);
        const auto pos = path.find_last_of('/');
        if (pos != std::string::npos) {
            return path.substr(0, pos);
        }
    }

    return ".";
}

}  // namespace

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

std::string Config::resolve_config_path(const char* argv0, const std::string& default_filename,
                                        const char* override_path) {
    const std::string base_dir = executable_dir(argv0);

    if (override_path == nullptr || override_path[0] == '\0') {
        return base_dir + "/config/" + default_filename;
    }

    std::string path = override_path;
    if (!path.empty() && path[0] == '/') {
        return path;
    }

    if (path.rfind("config/", 0) == 0) {
        return base_dir + "/" + path;
    }

    return base_dir + "/config/" + path;
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
