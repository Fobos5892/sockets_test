#include "Message.hpp"

#include <gtest/gtest.h>

TEST(MessageTest, IdMessageSerializeDeserializeRoundTrip) {
    Message<uint32_t> original;
    original.from_id = 1;
    original.recipient = 2;
    original.payload = "hello";

    const auto bytes = original.serialize();
    const Message<uint32_t> decoded = Message<uint32_t>::deserialize(bytes.data(), bytes.size());

    EXPECT_EQ(decoded.recipient, 2U);
    EXPECT_EQ(decoded.payload, "hello");
}

TEST(MessageTest, NicknameMessageSerializeDeserializeRoundTrip) {
    Message<std::string> original;
    original.from_id = 1;
    original.recipient = "bob";
    original.payload = "hello";

    const auto bytes = original.serialize();
    const Message<std::string> decoded = Message<std::string>::deserialize(bytes.data(), bytes.size());

    EXPECT_EQ(decoded.recipient, "bob");
    EXPECT_EQ(decoded.payload, "hello");
}

TEST(MessageTest, ParseIdMessageFromConsoleLine) {
    const auto message = parse_id_message("2:hello world", 1);
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->recipient, 2U);
    EXPECT_EQ(message->payload, "hello world");
    EXPECT_EQ(message->from_id, 1U);
}

TEST(MessageTest, ParseNicknameMessageFromConsoleLine) {
    const auto message = parse_nickname_message("bob:hello", 1);
    ASSERT_TRUE(message.has_value());
    EXPECT_EQ(message->recipient, "bob");
    EXPECT_EQ(message->payload, "hello");
}

TEST(MessageTest, NumericRecipientDetection) {
    EXPECT_TRUE(is_numeric_recipient("123"));
    EXPECT_FALSE(is_numeric_recipient(""));
    EXPECT_FALSE(is_numeric_recipient("12a"));
    EXPECT_FALSE(is_numeric_recipient("alice"));
}

TEST(MessageTest, InvalidConsoleLinesReturnNullopt) {
    EXPECT_FALSE(parse_id_message("alice:hello", 1).has_value());
    EXPECT_FALSE(parse_nickname_message("2:hello", 1).has_value());
    EXPECT_FALSE(parse_id_message("no-colon", 1).has_value());
}

TEST(MessageTest, TagMismatchThrows) {
    Message<uint32_t> message;
    message.recipient = 1;
    message.payload = "x";
    const auto bytes = message.serialize();
    EXPECT_THROW((void)Message<std::string>::deserialize(bytes.data(), bytes.size()), std::runtime_error);
}
