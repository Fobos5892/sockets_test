#include "viewmodel/InputCommandParser.hpp"

#include <gtest/gtest.h>

TEST(ClientParserTest, ParsesListUsers) {
    const auto command = client_viewmodel::parse_input_line("/users", 1);
    EXPECT_EQ(command.type, client_viewmodel::InputCommandType::ListUsers);
}

TEST(ClientParserTest, ParsesExit) {
    EXPECT_EQ(client_viewmodel::parse_input_line("/exit", 1).type, client_viewmodel::InputCommandType::Exit);
    EXPECT_EQ(client_viewmodel::parse_input_line("exit", 1).type, client_viewmodel::InputCommandType::Exit);
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

TEST(ClientParserTest, ParsesBroadcast) {
    const auto command = client_viewmodel::parse_input_line(
        std::string(protocol::kBroadcastRecipient) + ":hello everyone", 1);
    EXPECT_EQ(command.type, client_viewmodel::InputCommandType::ChatBroadcast);
    EXPECT_EQ(command.text, "hello everyone");
    EXPECT_TRUE(command.recipient_nickname.empty());
}

TEST(ClientParserTest, ParsesKeyOffer) {
    const auto command = client_viewmodel::parse_input_line("/key 2", 1);
    EXPECT_EQ(command.type, client_viewmodel::InputCommandType::KeyOffer);
    EXPECT_EQ(command.recipient_id, 2U);
}

TEST(ClientParserTest, RejectsInvalidLines) {
    EXPECT_EQ(client_viewmodel::parse_input_line("", 1).type, client_viewmodel::InputCommandType::Invalid);
    EXPECT_EQ(client_viewmodel::parse_input_line("nosep", 1).type, client_viewmodel::InputCommandType::Invalid);
}
