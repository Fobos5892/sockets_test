#pragma once

#include "view/IClientInputView.hpp"
#include "view/IClientOutputView.hpp"

class IClientView : public IClientInputView, public IClientOutputView {};
