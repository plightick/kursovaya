#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include "Account.h"
#include "Card.h"
#include "Transaction.h"
#include "FavoritePayment.h"

class UserBase {
public:
    virtual ~UserBase() = default;
    virtual bool isAdmin() const = 0;
    virtual const std::string &username() const = 0;
};

class RegularUser : public UserBase {
public:
    std::string usernameValue;
    std::string passwordHash;
    std::vector<Account> accounts;
    std::vector<Card> cards;
    std::vector<Transaction> history;
    std::vector<FavoritePayment> favorites;
    std::vector<std::string> notifications;

    RegularUser() = default;
    RegularUser(std::string uname, std::string pwhash)
        : usernameValue(std::move(uname)), passwordHash(std::move(pwhash)) {}

    bool isAdmin() const override { return false; }
    const std::string &username() const override { return usernameValue; }

    friend std::ostream &operator<<(std::ostream &os, const RegularUser &u) {
        os << u.usernameValue << "\n";
        os << u.passwordHash << "\n";
        os << u.accounts.size() << "\n";
        for (const auto &a : u.accounts) os << a << "\n";
        os << u.cards.size() << "\n";
        for (const auto &c : u.cards) os << c << "\n";
        os << u.history.size() << "\n";
        for (const auto &t : u.history) os << t << "\n";
        os << u.favorites.size() << "\n";
        for (const auto &f : u.favorites) os << f << "\n";
        os << u.notifications.size() << "\n";
        for (const auto &n : u.notifications) {
            std::string copy = n;
            std::replace(copy.begin(), copy.end(), '\n', ' ');
            os << copy << "\n";
        }
        return os;
    }

    friend std::istream &operator>>(std::istream &is, RegularUser &u) {
        std::string line;
        std::getline(is, u.usernameValue);
        std::getline(is, u.passwordHash);

        std::getline(is, line);
        std::size_t nAcc = line.empty() ? 0 : static_cast<std::size_t>(std::stoul(line));
        u.accounts.clear();
        for (std::size_t i = 0; i < nAcc; ++i) {
            Account a;
            std::string accLine;
            std::getline(is, accLine);
            std::istringstream as(accLine);
            as >> a;
            u.accounts.push_back(a);
        }

        std::getline(is, line);
        std::size_t nCards = line.empty() ? 0 : static_cast<std::size_t>(std::stoul(line));
        u.cards.clear();
        for (std::size_t i = 0; i < nCards; ++i) {
            Card c;
            std::string cardLine;
            std::getline(is, cardLine);
            std::istringstream cs(cardLine);
            cs >> c;
            u.cards.push_back(c);
        }

        std::getline(is, line);
        std::size_t nTx = line.empty() ? 0 : static_cast<std::size_t>(std::stoul(line));
        u.history.clear();
        for (std::size_t i = 0; i < nTx; ++i) {
            Transaction t;
            std::string txLine;
            std::getline(is, txLine);
            std::istringstream ts(txLine);
            ts >> t;
            u.history.push_back(t);
        }

        std::getline(is, line);
        std::size_t nFav = line.empty() ? 0 : static_cast<std::size_t>(std::stoul(line));
        u.favorites.clear();
        for (std::size_t i = 0; i < nFav; ++i) {
            FavoritePayment f;
            std::string favLine;
            std::getline(is, favLine);
            std::istringstream fs(favLine);
            fs >> f;
            u.favorites.push_back(f);
        }

        if (std::getline(is, line)) {
            std::size_t nNotif = line.empty() ? 0 : static_cast<std::size_t>(std::stoul(line));
            u.notifications.clear();
            for (std::size_t i = 0; i < nNotif; ++i) {
                std::string notifLine;
                std::getline(is, notifLine);
                u.notifications.push_back(notifLine);
            }
        } else {
            u.notifications.clear();
        }
        return is;
    }
};

class AdminUser : public UserBase {
public:
    std::string usernameValue;
    std::string passwordHash;
    bool isAdmin() const override { return true; }
    const std::string &username() const override { return usernameValue; }
};


