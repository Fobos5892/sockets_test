#pragma once

#include "Config.hpp"
#include "transport/IClientConnector.hpp"

class TcpClientConnector final : public IClientConnector {
public:
    int connect(const ClientConfig& config) override;
};
