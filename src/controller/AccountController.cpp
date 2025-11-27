#include "AccountController.h"
#include "../storage/UserStorage.h"
#include "../utils/Exceptions.h"
#include "../utils/Utils.h"

using namespace utils;
using namespace storage;

AccountController::AccountController(RegularUser &user, QObject *parent) : QObject(parent), currentUser(user) {}

QVariantList AccountController::listAccounts() const {
    QVariantList out;
    for (const auto &a : currentUser.accounts) {
        QVariantMap m;
        m["accountNumber"] = QString::fromStdString(a.accountNumber);
        m["currency"] = QString::fromStdString(a.currency);
        m["balanceCents"] = a.balanceCents;
        out.push_back(m);
    }
    return out;
}

QVariantList AccountController::listCards() const {
    QVariantList out;
    for (const auto &c : currentUser.cards) {
        QVariantMap m;
        m["cardNumber"] = QString::fromStdString(c.cardNumber);
        m["holderName"] = QString::fromStdString(c.holderName);
        m["expiry"] = QString::fromStdString(c.expiry);
        m["linkedAccount"] = QString::fromStdString(c.linkedAccount);
        out.push_back(m);
    }
    return out;
}

void AccountController::addAccount(const QString &currency) {
    try {
        Account a;
        a.accountNumber = generateNumericId(20);
        a.currency = currency.toStdString();
        a.balanceCents = 0;
        currentUser.accounts.push_back(a);
        saveCurrentUser();
        emit infoMessage("Счет добавлен");
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void AccountController::addCard([[maybe_unused]] const QString &holderName, const QString &expiry, const QString &linkedAccount) {
    try {
        bool accountExists = false;
        const std::string accId = linkedAccount.toStdString();
        for (const auto &a : currentUser.accounts) { if (a.accountNumber == accId) { accountExists = true; break; } }
        if (!accountExists) throw ValidationError("Нет такого счета");
        Card c;
        c.cardNumber = generateNumericId(16);
        c.holderName = currentUser.usernameValue;
        c.expiry = expiry.toStdString();
        c.linkedAccount = linkedAccount.toStdString();
        currentUser.cards.push_back(c);
        saveCurrentUser();
        emit infoMessage("Карта добавлена");
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void AccountController::saveCurrentUser() const {
    UserStorage::saveUser(currentUser);
}
