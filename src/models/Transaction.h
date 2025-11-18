#pragma once

#include <string>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <sstream>
#include "Payment.h"

class Transaction : public Payment {
public:
    std::string id;            // unique
    std::string fromAccount;   // account number
    std::string toCard;        // destination card or account
    long long cents = 0;
    std::time_t timestamp = 0;
    std::string note;
    std::string category = "other";  // medicine, sport, food, entertainment, other
    std::string status = "completed";
    std::string cancelReason;

    Transaction() = default;
    Transaction(std::string id_, std::string fromAcc, std::string to, long long c, std::time_t ts, std::string note_, std::string cat = "other")
        : id(std::move(id_)), fromAccount(std::move(fromAcc)), toCard(std::move(to)), cents(c), timestamp(ts), note(std::move(note_)), category(std::move(cat)) {}

    std::string description() const override { return note; }
    long long amountCents() const override { return cents; }

    friend std::ostream &operator<<(std::ostream &os, const Transaction &t) {
        auto sanitize = [](std::string value) {
            std::replace(value.begin(), value.end(), '\n', ' ');
            std::replace(value.begin(), value.end(), ',', ';');
            return value;
        };
        os << t.id << "," << t.fromAccount << "," << t.toCard << "," << t.cents << "," << t.timestamp << ","
           << sanitize(t.note) << "," << sanitize(t.category) << "," << sanitize(t.status) << "," << sanitize(t.cancelReason);
        return os;
    }

    friend std::istream &operator>>(std::istream &is, Transaction &t) {
        std::string line;
        if (!std::getline(is, line)) return is;
        std::stringstream ss(line);
        std::string field;
        auto desanitize = [](std::string value) {
            std::replace(value.begin(), value.end(), ';', ',');
            return value;
        };
        std::getline(ss, t.id, ',');
        std::getline(ss, t.fromAccount, ',');
        std::getline(ss, t.toCard, ',');
        std::getline(ss, field, ',');
        if (!field.empty()) t.cents = std::stoll(field);
        else t.cents = 0;
        std::getline(ss, field, ',');
        if (!field.empty()) t.timestamp = static_cast<std::time_t>(std::stoll(field));
        else t.timestamp = 0;
        if (std::getline(ss, field, ',')) t.note = desanitize(field); else t.note.clear();
        if (std::getline(ss, field, ',')) {
            t.category = field.empty() ? "other" : desanitize(field);
        } else {
            t.category = "other";
        }
        if (std::getline(ss, field, ',')) {
            t.status = field.empty() ? "completed" : desanitize(field);
        } else {
            t.status = "completed";
        }
        if (std::getline(ss, field, ',')) {
            t.cancelReason = desanitize(field);
        } else {
            t.cancelReason.clear();
        }
        return is;
    }
};


