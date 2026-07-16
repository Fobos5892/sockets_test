#include "viewmodel/IncomingMessagePresenter.hpp"

#include <gtest/gtest.h>

#include "testing/NullViews.hpp"

TEST(ClientPresenterTest, PresentsDeliver) {
    RecordingClientOutputView view;
    IncomingMessagePresenter presenter(view);

    protocol::AppMessage message;
    message.type = protocol::MsgType::Deliver;
    message.payload = protocol::DeliverPayload{7, "ping"};
    presenter.present(message);

    EXPECT_EQ(view.deliver_count, 1);
    EXPECT_EQ(view.last_deliver_from, 7U);
    EXPECT_EQ(view.last_deliver_text, "ping");
}

TEST(ClientPresenterTest, PresentsUsersList) {
    RecordingClientOutputView view;
    IncomingMessagePresenter presenter(view);

    protocol::AppMessage message;
    message.type = protocol::MsgType::UsersList;
    message.payload = protocol::UsersListPayload{{{1, "alice"}, {2, "bob"}}};
    presenter.present(message);

    ASSERT_EQ(view.users_list_count, 1);
    ASSERT_EQ(view.last_users.size(), 2U);
    EXPECT_EQ(view.last_users[0].nickname, "alice");
    EXPECT_EQ(view.last_users[1].nickname, "bob");
}

TEST(ClientPresenterTest, PresentsError) {
    RecordingClientOutputView view;
    IncomingMessagePresenter presenter(view);

    protocol::AppMessage message;
    message.type = protocol::MsgType::Error;
    message.payload = protocol::ErrorPayload{"boom"};
    presenter.present(message);

    EXPECT_EQ(view.error_count, 1);
    EXPECT_EQ(view.last_error, "boom");
}
