#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

class Config {
public:
    using ProgressCallback = std::function<void(int percent, const std::string& message)>;

    static Config load(const std::string& path, const ProgressCallback& progress = nullptr);
    static std::string resolve_config_path(const char* argv0, const std::string& default_filename,
                                           const char* override_path = nullptr);

    std::string get(const std::string& key, const std::string& default_value = "") const;

private:
    std::unordered_map<std::string, std::string> values_;
};

struct ServerConfig {
    std::string bind_ip;
    uint16_t port{5020};

    static ServerConfig from_file(const std::string& path,
                                  const Config::ProgressCallback& progress = nullptr);
};

struct ClientConfig {
    std::string server_ip;
    uint16_t port{5020};
    std::string nickname;

    static ClientConfig from_file(const std::string& path,
                                  const Config::ProgressCallback& progress = nullptr);
};
