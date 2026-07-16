#include <benchmark/benchmark.h>

#include <string>

#include "viewmodel/IncomingMessagePresenter.hpp"
#include "viewmodel/InputCommandParser.hpp"
#include "viewmodel/OutgoingMessageBuilder.hpp"
#include "testing/NullViews.hpp"
#include "testing/fixtures.hpp"

static void BM_ClientParseInputLine(benchmark::State& state) {
    const std::string line = state.range(0) == 0 ? "2:hello world" : "bob:hello world";
    for (auto _ : state) {
        auto command = client_viewmodel::parse_input_line(line, 1);
        benchmark::DoNotOptimize(command);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ClientParseInputLine)->Arg(0)->Arg(1);

static void BM_ClientBuildChatById(benchmark::State& state) {
    const std::string text(static_cast<size_t>(state.range(0)), 'c');
    for (auto _ : state) {
        auto payload = client_viewmodel::OutgoingMessageBuilder::build_chat_by_id(1, 2, text);
        benchmark::DoNotOptimize(payload.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ClientBuildChatById)->Arg(16)->Arg(256);

static void BM_ClientPresentDeliver(benchmark::State& state) {
    NullClientOutputView view;
    IncomingMessagePresenter presenter(view);
    const auto message = testing_fixtures::make_deliver_message(3, "hello");

    for (auto _ : state) {
        presenter.present(message);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ClientPresentDeliver);
