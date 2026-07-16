#pragma once

#include "Protocol.hpp"
#include "view/IClientOutputView.hpp"

class IncomingMessagePresenter {
public:
    explicit IncomingMessagePresenter(IClientOutputView& output_view);

    void present(const protocol::AppMessage& message) const;

private:
    IClientOutputView& output_view_;
};
