#pragma once

#include <string>
#include <iostream>

class Account {
public:
    std::string accountNumber; 
    std::string currency;      
    long long balanceCents = 0; 

    Account() = default;
    Account(std::string number, std::string curr, long long cents)
        : accountNumber(std::move(number)), currency(std::move(curr)), balanceCents(cents) {}

    bool operator<(const Account &other) const {
        return accountNumber < other.accountNumber;
    }

    friend std::ostream &operator<<(std::ostream &os, const Account &acc) {
        os << acc.accountNumber << "," << acc.currency << "," << acc.balanceCents;
        return os;
    }

    friend std::istream &operator>>(std::istream &is, Account &acc) {
        std::getline(is, acc.accountNumber, ',');
        std::getline(is, acc.currency, ',');
        std::string cents;
        std::getline(is, cents);
        if (!cents.empty()) acc.balanceCents = std::stoll(cents);
        return is;
    }
};


