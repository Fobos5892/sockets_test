#include "Config.hpp"

#include <gtest/gtest.h>

#include <climits>
#include <unistd.h>

#include "tests/test_utils.hpp"

TEST(ConfigTest, LoadsKeyValuePairs) {
    std::string path;
    test_utils::write_temp_config(
        "# comment\n"
        "server_ip=10.0.0.5\n"
        "port=6000\n"
        "nickname= tester \n",
        path);

    const Config config = Config::load(path);
    EXPECT_EQ(config.get("server_ip"), "10.0.0.5");
    EXPECT_EQ(config.get("port"), "6000");
    EXPECT_EQ(config.get("missing", "default"), "default");
}

TEST(ConfigTest, ServerConfigFromFile) {
    std::string path;
    test_utils::write_temp_config(
        "bind_ip=127.0.0.1\n"
        "port=5021\n",
        path);

    const ServerConfig config = ServerConfig::from_file(path);
    EXPECT_EQ(config.bind_ip, "127.0.0.1");
    EXPECT_EQ(config.port, 5021);
}

TEST(ConfigTest, ClientConfigFromFileUsesDefaults) {
    std::string path;
    test_utils::write_temp_config("nickname=bob\n", path);

    const ClientConfig config = ClientConfig::from_file(path);
    EXPECT_EQ(config.server_ip, "127.0.0.1");
    EXPECT_EQ(config.port, 5020);
    EXPECT_EQ(config.nickname, "bob");
}

TEST(ConfigTest, MissingFileThrows) {
    EXPECT_THROW(Config::load("/tmp/modbus_missing_config_12345.conf"), std::runtime_error);
}

TEST(ConfigTest, ResolveConfigPathUsesExecutableConfigDir) {
    char exe_path[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    ASSERT_GT(len, 0);
    exe_path[len] = '\0';

    std::string exe_dir(exe_path);
    const auto pos = exe_dir.find_last_of('/');
    ASSERT_NE(pos, std::string::npos);
    exe_dir = exe_dir.substr(0, pos);

    const std::string path = Config::resolve_config_path(exe_path, "server.conf", nullptr);
    EXPECT_EQ(path, exe_dir + "/config/server.conf");
}

TEST(ConfigTest, ResolveConfigPathAcceptsConfigFilename) {
    char exe_path[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    ASSERT_GT(len, 0);
    exe_path[len] = '\0';

    std::string exe_dir(exe_path);
    const auto pos = exe_dir.find_last_of('/');
    ASSERT_NE(pos, std::string::npos);
    exe_dir = exe_dir.substr(0, pos);

    const std::string path =
        Config::resolve_config_path(exe_path, "client.conf", "client2.conf");
    EXPECT_EQ(path, exe_dir + "/config/client2.conf");
}

TEST(ConfigTest, ResolveConfigPathAcceptsAbsolutePath) {
    const std::string path = Config::resolve_config_path("/tmp/out/Debug/client", "client.conf",
                                                         "/etc/modbus/client.conf");
    EXPECT_EQ(path, "/etc/modbus/client.conf");
}
