#include <benchmark/benchmark.h>

#include <memory>

#include "ModbusTcp.hpp"
#include "Protocol.hpp"
#include "model/ClientRegistry.hpp"
#include "testing/MemoryMessageTransport.hpp"
#include "testing/NullViews.hpp"
#include "testing/fixtures.hpp"
#include "viewmodel/IncomingMessagePresenter.hpp"
#include "viewmodel/InputCommandParser.hpp"
#include "viewmodel/MessageDispatcherFactory.hpp"
#include "viewmodel/OutgoingMessageBuilder.hpp"
#include "viewmodel/PresenceBroadcaster.hpp"
#include "viewmodel/ServerContext.hpp"

static void BM_E2E_ClientSendChatById(benchmark::State& state) {
    MemoryMessageTransport transport;
    constexpr int kFd = 1;

    for (auto _ : state) {
        const auto command = client_viewmodel::parse_input_line("2:hello", 1);
        const auto payload =
            client_viewmodel::OutgoingMessageBuilder::build_chat_by_id(1, command.recipient_id, command.text);
        bool ok = transport.send(kFd, protocol::MsgType::Chat, payload);
        auto message = transport.receive(kFd);
        benchmark::DoNotOptimize(ok);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_E2E_ClientSendChatById);

static void BM_E2E_ClientSendChatByNickname(benchmark::State& state) {
    MemoryMessageTransport transport;
    constexpr int kFd = 1;

    for (auto _ : state) {
        const auto command = client_viewmodel::parse_input_line("bob:hello", 1);
        const auto payload = client_viewmodel::OutgoingMessageBuilder::build_chat_by_nickname(
            1, command.recipient_nickname, command.text);
        bool ok = transport.send(kFd, protocol::MsgType::Chat, payload);
        auto message = transport.receive(kFd);
        benchmark::DoNotOptimize(ok);
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_E2E_ClientSendChatByNickname);

static void BM_E2E_ClientReceiveAndPresent(benchmark::State& state) {
    MemoryMessageTransport transport;
    NullClientOutputView view;
    IncomingMessagePresenter presenter(view);
    constexpr int kFd = 1;

    for (auto _ : state) {
        state.PauseTiming();
        transport.send(kFd, protocol::MsgType::Deliver, protocol::encode_deliver(3, "hello"));
        state.ResumeTiming();

        auto message = transport.receive(kFd);
        if (message) {
            presenter.present(*message);
        }
        benchmark::DoNotOptimize(message);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_E2E_ClientReceiveAndPresent);

static void BM_E2E_ClientServerRoundTrip(benchmark::State& state) {
    MemoryMessageTransport transport;
    NullServerView server_view;
    server_model::ClientRegistry registry;
    PresenceBroadcaster presence{registry, transport};
    ServerContext context{server_view, registry, transport, presence};
    auto dispatcher = create_message_dispatcher();

    registry.add(1, 10);
    registry.add(2, 20);
    registry.set_nickname(1, "alice");
    registry.set_nickname(2, "bob");

    NullClientOutputView client_view;
    IncomingMessagePresenter presenter(client_view);

    for (auto _ : state) {
        const auto command = client_viewmodel::parse_input_line("bob:hello", 10);
        const auto payload = client_viewmodel::OutgoingMessageBuilder::build_chat_by_nickname(
            10, command.recipient_nickname, command.text);

        const modbus::Frame frame = protocol::make_frame(protocol::MsgType::Chat, payload);
        const protocol::AppMessage inbound = protocol::decode_app_message(frame);
        dispatcher->dispatch(1, inbound, context);

        auto delivered = transport.receive(2);
        if (delivered) {
            presenter.present(*delivered);
        }
        benchmark::DoNotOptimize(delivered);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_E2E_ClientServerRoundTrip);
