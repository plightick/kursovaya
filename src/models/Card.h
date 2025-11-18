#pragma once

#include <string>
#include <iostream>

class Card {
public:
    std::string cardNumber; 
    std::string holderName;
    std::string expiry;
    std::string linkedAccount;

    friend std::ostream &operator<<(std::ostream &os, const Card &c) {
        os << c.cardNumber << "," << c.holderName << "," << c.expiry << "," << c.linkedAccount;
        return os;
    }

    friend std::istream &operator>>(std::istream &is, Card &c) {
        std::getline(is, c.cardNumber, ',');
        std::getline(is, c.holderName, ',');
        std::getline(is, c.expiry, ',');
        std::getline(is, c.linkedAccount);
        return is;
    }
};
