#include "transport/ModbusMessageTransport.hpp"

#include "ModbusTcp.hpp"

bool ModbusMessageTransport::send(int fd, protocol::MsgType type, const std::vector<uint8_t>& payload) {
    const modbus::Frame frame = protocol::make_frame(type, payload);
    return modbus::ModbusTcp::write_frame(fd, frame);
}

std::optional<protocol::AppMessage> ModbusMessageTransport::receive(int fd) {
    std::vector<uint8_t> buffer;
    modbus::Frame frame;
    if (!modbus::ModbusTcp::read_frame(fd, buffer, frame)) {
        return std::nullopt;
    }
    return protocol::decode_app_message(frame);
}
