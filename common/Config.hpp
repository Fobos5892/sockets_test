#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace config_defaults {

constexpr uint16_t kDefaultPort = 5020;
constexpr const char* kDefaultBindIp = "0.0.0.0";
constexpr const char* kDefaultServerIp = "127.0.0.1";
constexpr const char* kConfSuffix = ".conf";
constexpr const char* kDefaultServerConfigName = "server.conf";
constexpr const char* kDefaultClientConfigName = "client.conf";
constexpr const char* kDefaultKeystorePassword = "modbus-chat";
constexpr const char* kDefaultClientLogName = "client.log";

}  // namespace config_defaults

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
    uint16_t port{config_defaults::kDefaultPort};

    static ServerConfig from_file(const std::string& path,
                                  const Config::ProgressCallback& progress = nullptr);
};

struct ClientConfig {
    std::string server_ip;
    uint16_t port{config_defaults::kDefaultPort};
    std::string nickname;
    std::string keystore_path;
    std::string keystore_password{config_defaults::kDefaultKeystorePassword};
    std::string log_path;

    static ClientConfig from_file(const std::string& path,
                                  const Config::ProgressCallback& progress = nullptr);
};
