#include <benchmark/benchmark.h>

#include <string>
#include <vector>

#include "Protocol.hpp"
#include "testing/fixtures.hpp"

static void BM_ProtocolChatRoundTrip(benchmark::State& state) {
    const std::string text(static_cast<size_t>(state.range(0)), 'x');
    const auto chat = testing_fixtures::make_chat_by_id(1, 2, text);
    for (auto _ : state) {
        const auto payload = protocol::encode_chat(chat);
        const auto frame = protocol::make_frame(protocol::MsgType::Chat, payload);
        auto message = protocol::decode_app_message(frame);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ProtocolChatRoundTrip)->Arg(16)->Arg(256)->Arg(2048);

static void BM_ProtocolUsersListRoundTrip(benchmark::State& state) {
    std::vector<protocol::UserInfo> users;
    users.reserve(static_cast<size_t>(state.range(0)));
    for (int i = 0; i < state.range(0); ++i) {
        users.push_back({static_cast<uint32_t>(i + 1), "user" + std::to_string(i)});
    }

    for (auto _ : state) {
        const auto payload = protocol::encode_users_list(users);
        const auto frame = protocol::make_frame(protocol::MsgType::UsersList, payload);
        auto message = protocol::decode_app_message(frame);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ProtocolUsersListRoundTrip)->Arg(10)->Arg(100)->Arg(1000);
