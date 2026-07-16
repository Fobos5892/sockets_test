#pragma once

#include "Config.hpp"

class IClientConnector {
public:
    virtual ~IClientConnector() = default;

    virtual int connect(const ClientConfig& config) = 0;
};
