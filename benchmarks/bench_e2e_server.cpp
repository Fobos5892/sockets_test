#include <benchmark/benchmark.h>

#include <memory>

#include "model/ClientRegistry.hpp"
#include "testing/MemoryMessageTransport.hpp"
#include "testing/NullViews.hpp"
#include "testing/fixtures.hpp"
#include "viewmodel/MessageDispatcherFactory.hpp"
#include "viewmodel/PresenceBroadcaster.hpp"
#include "viewmodel/ServerContext.hpp"

namespace {

struct ServerE2EFixture {
    NullServerView view;
    server_model::ClientRegistry registry;
    MemoryMessageTransport transport;
    PresenceBroadcaster presence{registry, transport};
    ServerContext context{view, registry, transport, presence};
    std::unique_ptr<MessageDispatcher> dispatcher{create_message_dispatcher()};

    ServerE2EFixture() {
        registry.add(1, 10);
        registry.add(2, 20);
        registry.set_nickname(1, "alice");
        registry.set_nickname(2, "bob");
    }
};

}  // namespace

static void BM_E2E_ServerChatById(benchmark::State& state) {
    ServerE2EFixture fx;
    const auto chat = testing_fixtures::make_chat_by_id(10, 20, "hello");
    const auto message = testing_fixtures::make_chat_message(chat);

    for (auto _ : state) {
        fx.dispatcher->dispatch(1, message, fx.context);
        auto delivered = fx.transport.receive(2);
        benchmark::DoNotOptimize(delivered);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_E2E_ServerChatById);

static void BM_E2E_ServerChatByNickname(benchmark::State& state) {
    ServerE2EFixture fx;
    const auto chat = testing_fixtures::make_chat_by_nickname(10, "bob", "hello");
    const auto message = testing_fixtures::make_chat_message(chat);

    for (auto _ : state) {
        fx.dispatcher->dispatch(1, message, fx.context);
        auto delivered = fx.transport.receive(2);
        benchmark::DoNotOptimize(delivered);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_E2E_ServerChatByNickname);

static void BM_E2E_ServerListUsers(benchmark::State& state) {
    ServerE2EFixture fx;
    const auto message = testing_fixtures::make_list_users_message();

    for (auto _ : state) {
        fx.dispatcher->dispatch(1, message, fx.context);
        auto list = fx.transport.receive(1);
        benchmark::DoNotOptimize(list);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_E2E_ServerListUsers);
