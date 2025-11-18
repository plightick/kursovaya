#pragma once

#include <string>

class Payment {
public:
    virtual ~Payment() = default;
    virtual std::string description() const = 0;
    virtual long long amountCents() const = 0;
};


