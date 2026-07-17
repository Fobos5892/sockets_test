#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace modbus {

constexpr uint8_t kAppFunctionCode = 0x41;

// MBAP: transaction(2) + protocol(2) + length(2) = 6; + unit id = 7.
constexpr size_t kMbapHeaderSize = 6;
constexpr size_t kMbapUnitHeaderSize = 7;
constexpr size_t kMbapLengthFieldOffset = 4;
constexpr size_t kUnitIdLengthContribution = 1;
constexpr uint16_t kProtocolId = 0;
constexpr int kPollWaitForever = -1;

struct Frame {
    uint16_t transaction_id{0};
    uint8_t unit_id{0};
    std::vector<uint8_t> pdu;
};

class ModbusTcp {
public:
    static std::vector<uint8_t> encode(const Frame& frame);
    static std::optional<Frame> decode(const std::vector<uint8_t>& buffer, size_t& consumed);

    static bool read_frame(int fd, std::vector<uint8_t>& buffer, Frame& out);
    static bool write_frame(int fd, const Frame& frame);
};

}  // namespace modbus
