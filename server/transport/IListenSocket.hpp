#pragma once

#include <cstdint>

class IListenSocket {
public:
    virtual ~IListenSocket() = default;

    virtual void open() = 0;
    virtual void close() = 0;
    virtual int fd() const = 0;
    virtual int accept() = 0;
    virtual uint16_t port() const = 0;
    virtual bool is_open() const = 0;
};
