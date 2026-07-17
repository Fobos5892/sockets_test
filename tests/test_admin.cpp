#include "viewmodel/AdminCommandParser.hpp"

#include <gtest/gtest.h>

TEST(AdminCommandParserTest, ParsesBanList) {
    const auto command = server_admin::parse_admin_command("ban:1, 2,3");
    ASSERT_TRUE(command.has_value());
    ASSERT_EQ(command->ids.size(), 3u);
    EXPECT_EQ(command->ids[0], 1u);
    EXPECT_EQ(command->ids[1], 2u);
    EXPECT_EQ(command->ids[2], 3u);
}

TEST(AdminCommandParserTest, RejectsInvalid) {
    EXPECT_FALSE(server_admin::parse_admin_command("hello").has_value());
    EXPECT_FALSE(server_admin::parse_admin_command("ban:").has_value());
    EXPECT_FALSE(server_admin::parse_admin_command("ban:abc").has_value());
    EXPECT_FALSE(server_admin::parse_admin_command("ban:0").has_value());
}

TEST(AdminCommandParserTest, HelpTextMentionsBan) {
    EXPECT_NE(server_admin::admin_help_text().find("ban:"), std::string::npos);
}
