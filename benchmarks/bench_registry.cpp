#include <benchmark/benchmark.h>

#include <string>

#include "model/ClientRegistry.hpp"
#include "testing/fixtures.hpp"

static void BM_RegistryResolveById(benchmark::State& state) {
    server_model::ClientRegistry registry;
    const int n = static_cast<int>(state.range(0));
    for (int i = 1; i <= n; ++i) {
        registry.add(i, static_cast<uint32_t>(i));
        registry.set_nickname(i, "user" + std::to_string(i));
    }
    const auto chat = testing_fixtures::make_chat_by_id(1, static_cast<uint32_t>(n), "hi");

    for (auto _ : state) {
        auto id = registry.resolve_recipient(chat);
        benchmark::DoNotOptimize(id);
    }
}
BENCHMARK(BM_RegistryResolveById)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_RegistryResolveByNickname(benchmark::State& state) {
    server_model::ClientRegistry registry;
    const int n = static_cast<int>(state.range(0));
    for (int i = 1; i <= n; ++i) {
        registry.add(i, static_cast<uint32_t>(i));
        registry.set_nickname(i, "user" + std::to_string(i));
    }
    const auto chat = testing_fixtures::make_chat_by_nickname(1, "user" + std::to_string(n), "hi");

    for (auto _ : state) {
        auto id = registry.resolve_recipient(chat);
        benchmark::DoNotOptimize(id);
    }
}
BENCHMARK(BM_RegistryResolveByNickname)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_RegistryCollectUsers(benchmark::State& state) {
    server_model::ClientRegistry registry;
    const int n = static_cast<int>(state.range(0));
    for (int i = 1; i <= n; ++i) {
        registry.add(i, static_cast<uint32_t>(i));
        registry.set_nickname(i, "user" + std::to_string(i));
    }

    for (auto _ : state) {
        auto users = registry.collect_users();
        benchmark::DoNotOptimize(users.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_RegistryCollectUsers)->Arg(100)->Arg(1000)->Arg(10000);
