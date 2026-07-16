#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "model/ClientSession.hpp"
#include "transport/IClientConnector.hpp"
#include "transport/IMessageTransport.hpp"
#include "view/IClientInputView.hpp"
#include "view/IClientOutputView.hpp"
#include "viewmodel/IncomingMessagePresenter.hpp"
#include "viewmodel/InputCommandParser.hpp"

class ClientViewModel {
public:
    ClientViewModel(client_model::ClientSession session, IClientInputView& input_view,
                    IClientOutputView& output_view, std::unique_ptr<IClientConnector> connector,
                    std::unique_ptr<IMessageTransport> transport);
    ~ClientViewModel();

    void run();

private:
    void connect_to_server();
    void receive_assigned_id();
    void register_nickname();
    void input_loop();
    void receive_loop();
    void handle_input_command(const client_viewmodel::InputCommand& command);
    void shutdown();

    client_model::ClientSession session_;
    IClientInputView& input_view_;
    IClientOutputView& output_view_;
    std::unique_ptr<IClientConnector> connector_;
    std::unique_ptr<IMessageTransport> transport_;
    IncomingMessagePresenter presenter_;
    std::atomic<bool> running_{true};
    std::thread receiver_;
};
