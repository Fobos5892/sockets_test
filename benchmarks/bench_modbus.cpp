#include <benchmark/benchmark.h>

#include <string>
#include <vector>

#include "ModbusTcp.hpp"
#include "Protocol.hpp"

namespace {

modbus::Frame make_sample_frame(size_t payload_size) {
    std::vector<uint8_t> payload(payload_size, 0xAB);
    return protocol::make_frame(protocol::MsgType::Chat, payload, 42);
}

}  // namespace

static void BM_ModbusEncode(benchmark::State& state) {
    const modbus::Frame frame = make_sample_frame(static_cast<size_t>(state.range(0)));
    for (auto _ : state) {
        auto encoded = modbus::ModbusTcp::encode(frame);
        benchmark::DoNotOptimize(encoded.data());
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(state.range(0)));
}
BENCHMARK(BM_ModbusEncode)->Arg(16)->Arg(256)->Arg(4096);

static void BM_ModbusDecode(benchmark::State& state) {
    const modbus::Frame frame = make_sample_frame(static_cast<size_t>(state.range(0)));
    const auto encoded = modbus::ModbusTcp::encode(frame);
    for (auto _ : state) {
        size_t consumed = 0;
        auto decoded = modbus::ModbusTcp::decode(encoded, consumed);
        benchmark::DoNotOptimize(decoded);
        benchmark::DoNotOptimize(consumed);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ModbusDecode)->Arg(16)->Arg(256)->Arg(4096);

static void BM_ModbusEncodeDecodeRoundTrip(benchmark::State& state) {
    const modbus::Frame frame = make_sample_frame(static_cast<size_t>(state.range(0)));
    for (auto _ : state) {
        const auto encoded = modbus::ModbusTcp::encode(frame);
        size_t consumed = 0;
        auto decoded = modbus::ModbusTcp::decode(encoded, consumed);
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ModbusEncodeDecodeRoundTrip)->Arg(64)->Arg(1024);
