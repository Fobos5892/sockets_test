#include "ModbusTcp.hpp"

#include <gtest/gtest.h>

#include <sys/socket.h>
#include <unistd.h>

TEST(ModbusTcpTest, EncodeDecodeRoundTrip) {
    modbus::Frame original;
    original.transaction_id = 42;
    original.unit_id = 1;
    original.pdu = {modbus::kAppFunctionCode, 0x03, 0x00, 0x01, 0x02};

    const auto encoded = modbus::ModbusTcp::encode(original);
    size_t consumed = 0;
    const auto decoded = modbus::ModbusTcp::decode(encoded, consumed);

    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(consumed, encoded.size());
    EXPECT_EQ(decoded->transaction_id, 42);
    EXPECT_EQ(decoded->unit_id, 1);
    EXPECT_EQ(decoded->pdu, original.pdu);
}

TEST(ModbusTcpTest, DecodeReturnsNulloptForIncompleteBuffer) {
    modbus::Frame frame;
    frame.pdu = {0x41, 0x01};
    const auto encoded = modbus::ModbusTcp::encode(frame);

    std::vector<uint8_t> partial(encoded.begin(), encoded.end() - 1);
    size_t consumed = 0;
    EXPECT_FALSE(modbus::ModbusTcp::decode(partial, consumed).has_value());
}

TEST(ModbusTcpTest, ReadWriteFrameOverSocketPair) {
    int fds[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    modbus::Frame sent;
    sent.transaction_id = 7;
    sent.unit_id = 0;
    sent.pdu = {modbus::kAppFunctionCode, 0x02, 0x00, 0x00, 0x00, 0x01};

    ASSERT_TRUE(modbus::ModbusTcp::write_frame(fds[0], sent));

    modbus::Frame received;
    std::vector<uint8_t> buffer;
    ASSERT_TRUE(modbus::ModbusTcp::read_frame(fds[1], buffer, received));
    EXPECT_EQ(received.transaction_id, 7);
    EXPECT_EQ(received.pdu, sent.pdu);

    close(fds[0]);
    close(fds[1]);
}
