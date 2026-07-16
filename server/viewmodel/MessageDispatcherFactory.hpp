#pragma once

#include <memory>

#include "viewmodel/MessageDispatcher.hpp"

std::unique_ptr<MessageDispatcher> create_message_dispatcher();
