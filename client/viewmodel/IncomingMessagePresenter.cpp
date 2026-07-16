#include "viewmodel/IncomingMessagePresenter.hpp"

IncomingMessagePresenter::IncomingMessagePresenter(IClientOutputView& output_view) : output_view_(output_view) {}

void IncomingMessagePresenter::present(const protocol::AppMessage& message) const {
    switch (message.type) {
        case protocol::MsgType::Deliver: {
            const auto& deliver = std::get<protocol::DeliverPayload>(message.payload);
            output_view_.show_delivered(deliver.from_id, deliver.text);
            break;
        }
        case protocol::MsgType::UserJoined: {
            const auto& joined = std::get<protocol::UserJoinedPayload>(message.payload);
            output_view_.show_user_joined(joined.id, joined.nickname);
            break;
        }
        case protocol::MsgType::UserLeft: {
            const auto& left = std::get<protocol::UserLeftPayload>(message.payload);
            output_view_.show_user_left(left.id, left.nickname);
            break;
        }
        case protocol::MsgType::UsersList: {
            const auto& list = std::get<protocol::UsersListPayload>(message.payload);
            output_view_.show_users_list(list.users);
            break;
        }
        case protocol::MsgType::Error: {
            const auto& error = std::get<protocol::ErrorPayload>(message.payload);
            output_view_.show_error(error.text);
            break;
        }
        default:
            break;
    }
}
