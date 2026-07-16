#include <benchmark/benchmark.h>

#include <string>

#include "Message.hpp"

static void BM_MessageIdSerializeDeserialize(benchmark::State& state) {
    Message<uint32_t> message;
    message.from_id = 1;
    message.recipient = 2;
    message.payload = std::string(static_cast<size_t>(state.range(0)), 'm');

    for (auto _ : state) {
        const auto bytes = message.serialize();
        auto decoded = Message<uint32_t>::deserialize(bytes.data(), bytes.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_MessageIdSerializeDeserialize)->Arg(16)->Arg(256);

static void BM_MessageNicknameSerializeDeserialize(benchmark::State& state) {
    Message<std::string> message;
    message.from_id = 1;
    message.recipient = "bob";
    message.payload = std::string(static_cast<size_t>(state.range(0)), 'n');

    for (auto _ : state) {
        const auto bytes = message.serialize();
        auto decoded = Message<std::string>::deserialize(bytes.data(), bytes.size());
        benchmark::DoNotOptimize(decoded);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_MessageNicknameSerializeDeserialize)->Arg(16)->Arg(256);
