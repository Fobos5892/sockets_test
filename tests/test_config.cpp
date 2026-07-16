#include "Config.hpp"

#include <gtest/gtest.h>

#include "tests/test_utils.hpp"

TEST(ConfigTest, LoadsKeyValuePairs) {
    const std::string path = test_utils::write_temp_config(
        "# comment\n"
        "server_ip=10.0.0.5\n"
        "port=6000\n"
        "nickname= tester \n");

    const Config config = Config::load(path);
    EXPECT_EQ(config.get("server_ip"), "10.0.0.5");
    EXPECT_EQ(config.get("port"), "6000");
    EXPECT_EQ(config.get("missing", "default"), "default");
}

TEST(ConfigTest, ServerConfigFromFile) {
    const std::string path = test_utils::write_temp_config(
        "bind_ip=127.0.0.1\n"
        "port=5021\n");

    const ServerConfig config = ServerConfig::from_file(path);
    EXPECT_EQ(config.bind_ip, "127.0.0.1");
    EXPECT_EQ(config.port, 5021);
}

TEST(ConfigTest, ClientConfigFromFileUsesDefaults) {
    const std::string path = test_utils::write_temp_config("nickname=bob\n");

    const ClientConfig config = ClientConfig::from_file(path);
    EXPECT_EQ(config.server_ip, "127.0.0.1");
    EXPECT_EQ(config.port, 5020);
    EXPECT_EQ(config.nickname, "bob");
}

TEST(ConfigTest, MissingFileThrows) {
    EXPECT_THROW(Config::load("/tmp/modbus_missing_config_12345.conf"), std::runtime_error);
}
