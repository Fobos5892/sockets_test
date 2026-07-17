#pragma once

template <typename Target>
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute(Target& target) = 0;
};
