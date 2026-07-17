#include "view/QueuedClientOutputView.hpp"
#include "viewmodel/IncomingMessagePresenter.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace {

class CountingOutputView final : public IClientOutputView {
public:
    void show_status(const std::string&, int) override {}
    void show_input_ready() override {}
    void show_input_prompt() override {}
    void show_connected(const std::string&, uint16_t) override { ++connected; }
    void show_my_id(uint32_t) override {}
    void show_usage() override {}
    void show_delivered(uint32_t from_id, const std::string& text) override {
        last_from = from_id;
        last_text = text;
        ++delivered;
    }
    void show_user_joined(uint32_t, const std::string&) override {}
    void show_user_left(uint32_t, const std::string&) override {}
    void show_users_list(const std::vector<protocol::UserInfo>&) override {}
    void show_error(const std::string&) override { ++errors; }
    void show_send_failed() override { ++send_failed; }
    void show_invalid_format() override {}
    void show_decode_error(const std::string&) override {}

    std::atomic<int> connected{0};
    std::atomic<int> delivered{0};
    std::atomic<int> errors{0};
    std::atomic<int> send_failed{0};
    std::atomic<uint32_t> last_from{0};
    std::string last_text;
};

bool wait_for(const std::atomic<int>& counter, int expected, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (counter.load() < expected) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

}  // namespace

TEST(QueuedClientOutputViewTest, SerializesCommandsFromMultipleThreads) {
    CountingOutputView target;
    QueuedClientOutputView queued(target);

    constexpr int kPosts = 50;
    std::thread producer([&]() {
        for (int i = 0; i < kPosts; ++i) {
            queued.show_delivered(static_cast<uint32_t>(i), "msg");
        }
    });

    for (int i = 0; i < kPosts; ++i) {
        queued.show_send_failed();
    }

    producer.join();
    ASSERT_TRUE(wait_for(target.delivered, kPosts, std::chrono::seconds(2)));
    ASSERT_TRUE(wait_for(target.send_failed, kPosts, std::chrono::seconds(2)));

    queued.stop();
    EXPECT_EQ(target.delivered.load(), kPosts);
    EXPECT_EQ(target.send_failed.load(), kPosts);
}

TEST(QueuedClientOutputViewTest, PresenterGoesThroughQueue) {
    CountingOutputView target;
    QueuedClientOutputView queued(target);
    IncomingMessagePresenter presenter(queued);

    protocol::AppMessage message;
    message.type = protocol::MsgType::Deliver;
    message.payload = protocol::DeliverPayload{9, "hello"};
    presenter.present(message);

    ASSERT_TRUE(wait_for(target.delivered, 1, std::chrono::seconds(2)));
    EXPECT_EQ(target.last_from.load(), 9U);

    queued.stop();
}
