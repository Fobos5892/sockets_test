#pragma once

#include <optional>
#include <string>

class IClientInputView {
public:
    virtual ~IClientInputView() = default;

    virtual std::optional<std::string> read_line() = 0;
};
