#include "Config.hpp"

#include <algorithm>
#include <climits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace {

constexpr int kProgressComplete = 100;

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

bool has_conf_suffix(const std::string& path) {
    const std::string suffix = config_defaults::kConfSuffix;
    return path.size() >= suffix.size() &&
           path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0;
}

}  // namespace

Config Config::load(const std::string& path, const ProgressCallback& progress) {
    Config config;
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + path);
    }

    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (progress) {
        progress(0, "Opening " + path);
    }

    std::string line;
    std::streamoff consumed = 0;
    int last_percent = -1;
    while (std::getline(file, line)) {
        consumed += static_cast<std::streamoff>(line.size() + 1);
        if (progress && size > 0) {
            const int percent = static_cast<int>((consumed * kProgressComplete) / size);
            if (percent != last_percent) {
                last_percent = percent;
                progress(std::min(percent, kProgressComplete), "Reading config");
            }
        }

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

    if (progress) {
        progress(kProgressComplete, "Config loaded");
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

    if (!has_conf_suffix(path)) {
        path += config_defaults::kConfSuffix;
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

ServerConfig ServerConfig::from_file(const std::string& path, const Config::ProgressCallback& progress) {
    const Config config = Config::load(path, progress);
    ServerConfig server;
    server.bind_ip = config.get("bind_ip", config_defaults::kDefaultBindIp);
    server.port = static_cast<uint16_t>(
        std::stoi(config.get("port", std::to_string(config_defaults::kDefaultPort))));
    return server;
}

ClientConfig ClientConfig::from_file(const std::string& path, const Config::ProgressCallback& progress) {
    const Config config = Config::load(path, progress);
    ClientConfig client;
    client.server_ip = config.get("server_ip", config_defaults::kDefaultServerIp);
    client.port = static_cast<uint16_t>(
        std::stoi(config.get("port", std::to_string(config_defaults::kDefaultPort))));
    client.nickname = config.get("nickname", "");
    client.keystore_path = config.get("keystore_path", "");
    client.keystore_password =
        config.get("keystore_password", config_defaults::kDefaultKeystorePassword);
    client.log_path = config.get("log_path", "");
    return client;
}
