#include "ModbusTcp.hpp"

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

namespace modbus {

namespace {

uint16_t read_u16_be(const uint8_t* data) {
    uint16_t value = 0;
    std::memcpy(&value, data, sizeof(value));
    return ntohs(value);
}

void write_u16_be(std::vector<uint8_t>& out, uint16_t value) {
    const uint16_t net = htons(value);
    const auto* bytes = reinterpret_cast<const uint8_t*>(&net);
    out.insert(out.end(), bytes, bytes + sizeof(net));
}

bool read_exact(int fd, uint8_t* buffer, size_t size) {
    size_t total = 0;
    while (total < size) {
        pollfd pfd{};
        pfd.fd = fd;
        pfd.events = POLLIN;

        const int ready = poll(&pfd, 1, kPollWaitForever);
        if (ready <= 0) {
            return false;
        }
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            return false;
        }

        const ssize_t n = read(fd, buffer + total, size - total);
        if (n <= 0) {
            return false;
        }
        total += static_cast<size_t>(n);
    }
    return true;
}

bool write_exact(int fd, const uint8_t* buffer, size_t size) {
    size_t total = 0;
    while (total < size) {
        const ssize_t n = ::send(fd, buffer + total, size - total, MSG_NOSIGNAL);
        if (n <= 0) {
            return false;
        }
        total += static_cast<size_t>(n);
    }
    return true;
}

}  // namespace

std::vector<uint8_t> ModbusTcp::encode(const Frame& frame) {
    std::vector<uint8_t> out;
    out.reserve(kMbapUnitHeaderSize + frame.pdu.size());

    write_u16_be(out, frame.transaction_id);
    write_u16_be(out, kProtocolId);
    write_u16_be(out, static_cast<uint16_t>(frame.pdu.size() + kUnitIdLengthContribution));
    out.push_back(frame.unit_id);
    out.insert(out.end(), frame.pdu.begin(), frame.pdu.end());
    return out;
}

std::optional<Frame> ModbusTcp::decode(const std::vector<uint8_t>& buffer, size_t& consumed) {
    if (buffer.size() < kMbapUnitHeaderSize) {
        return std::nullopt;
    }

    const uint16_t length = read_u16_be(buffer.data() + kMbapLengthFieldOffset);
    const size_t total = kMbapHeaderSize + length;
    if (buffer.size() < total) {
        return std::nullopt;
    }

    Frame frame;
    frame.transaction_id = read_u16_be(buffer.data());
    frame.unit_id = buffer[kMbapHeaderSize];
    frame.pdu.assign(buffer.begin() + kMbapUnitHeaderSize, buffer.begin() + total);
    consumed = total;
    return frame;
}

bool ModbusTcp::read_frame(int fd, std::vector<uint8_t>& buffer, Frame& out) {
    uint8_t header[kMbapHeaderSize]{};
    if (!read_exact(fd, header, sizeof(header))) {
        return false;
    }

    const uint16_t length = read_u16_be(header + kMbapLengthFieldOffset);
    if (length == 0) {
        return false;
    }

    std::vector<uint8_t> rest(length);
    if (!read_exact(fd, rest.data(), rest.size())) {
        return false;
    }

    buffer.assign(header, header + sizeof(header));
    buffer.insert(buffer.end(), rest.begin(), rest.end());

    size_t consumed = 0;
    const auto frame = decode(buffer, consumed);
    if (!frame || consumed != buffer.size()) {
        return false;
    }

    out = *frame;
    return true;
}

bool ModbusTcp::write_frame(int fd, const Frame& frame) {
    const auto encoded = encode(frame);
    return write_exact(fd, encoded.data(), encoded.size());
}

}  // namespace modbus
