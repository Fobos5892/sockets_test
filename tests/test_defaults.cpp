#include "StartupProgress.hpp"

#include <gtest/gtest.h>

#include "Config.hpp"
#include "Protocol.hpp"

TEST(DefaultsTest, ConfigDefaultsAreStable) {
    EXPECT_EQ(config_defaults::kDefaultPort, 5020);
    EXPECT_STREQ(config_defaults::kDefaultBindIp, "0.0.0.0");
    EXPECT_STREQ(config_defaults::kDefaultServerIp, "127.0.0.1");
    EXPECT_STREQ(config_defaults::kConfSuffix, ".conf");
    EXPECT_STREQ(config_defaults::kDefaultServerConfigName, "server.conf");
    EXPECT_STREQ(config_defaults::kDefaultClientConfigName, "client.conf");
}

TEST(DefaultsTest, ProtocolRecipientTags) {
    EXPECT_EQ(protocol::kRecipientById, 0x01);
    EXPECT_EQ(protocol::kRecipientByNickname, 0x02);
    EXPECT_EQ(protocol::kRecipientBroadcast, 0x03);
    EXPECT_STREQ(protocol::kBroadcastRecipient, "all");
}

TEST(DefaultsTest, ProtocolWireSizes) {
    EXPECT_EQ(protocol::kUint16Size, 2U);
    EXPECT_EQ(protocol::kUint32Size, 4U);
    EXPECT_EQ(protocol::kPduAppHeaderSize, 2U);
    EXPECT_EQ(protocol::kChatHeaderSize, protocol::kUint32Size + 1);
}

TEST(DefaultsTest, StartupProgressMapping) {
    EXPECT_EQ(client_startup::map_config_file_percent(0), client_startup::kReadConfigBeginPercent);
    EXPECT_EQ(client_startup::map_config_file_percent(100),
              client_startup::kReadConfigBeginPercent + client_startup::kConfigReadBandWidth);
    EXPECT_EQ(client_startup::map_config_file_percent(50),
              client_startup::kReadConfigBeginPercent +
                  (50 * client_startup::kConfigReadBandWidth) / client_startup::kProgressMax);
}

TEST(DefaultsTest, StartupProgressBandsAreOrdered) {
    EXPECT_LT(client_startup::kResolveConfigPercent, client_startup::kReadConfigBeginPercent);
    EXPECT_LT(client_startup::kReadConfigBeginPercent, client_startup::kConfigReadyPercent);
    EXPECT_LT(client_startup::kConfigReadyPercent, client_startup::kConnectingPercent);
    EXPECT_LT(client_startup::kConnectingPercent, client_startup::kTcpConnectedPercent);
    EXPECT_LT(client_startup::kTcpConnectedPercent, client_startup::kWaitAssignIdPercent);
    EXPECT_LT(client_startup::kWaitAssignIdPercent, client_startup::kGotIdPercent);
    EXPECT_LT(client_startup::kGotIdPercent, client_startup::kRegisteringPercent);
    EXPECT_LT(client_startup::kRegisteringPercent, client_startup::kCompletePercent);
    EXPECT_EQ(client_startup::kCompletePercent, client_startup::kProgressMax);
}
