#include "viewmodel/InputCommandParser.hpp"

#include <gtest/gtest.h>

TEST(ClientParserTest, ParsesListUsers) {
    const auto command = client_viewmodel::parse_input_line("/users", 1);
    EXPECT_EQ(command.type, client_viewmodel::InputCommandType::ListUsers);
}

TEST(ClientParserTest, ParsesChatById) {
    const auto command = client_viewmodel::parse_input_line("2:hello", 1);
    EXPECT_EQ(command.type, client_viewmodel::InputCommandType::ChatById);
    EXPECT_EQ(command.recipient_id, 2U);
    EXPECT_EQ(command.text, "hello");
}

TEST(ClientParserTest, ParsesChatByNickname) {
    const auto command = client_viewmodel::parse_input_line("bob:hi there", 1);
    EXPECT_EQ(command.type, client_viewmodel::InputCommandType::ChatByNickname);
    EXPECT_EQ(command.recipient_nickname, "bob");
    EXPECT_EQ(command.text, "hi there");
}

TEST(ClientParserTest, RejectsInvalidLines) {
    EXPECT_EQ(client_viewmodel::parse_input_line("", 1).type, client_viewmodel::InputCommandType::Invalid);
    EXPECT_EQ(client_viewmodel::parse_input_line("nosep", 1).type, client_viewmodel::InputCommandType::Invalid);
}
