#pragma once

#include "concurrency/ICommand.hpp"
#include "view/IClientOutputView.hpp"

using IClientOutputCommand = ICommand<IClientOutputView>;
