#include "AdminController.h"
#include "../storage/UserStorage.h"
#include "../utils/Exceptions.h"
#include "../utils/Utils.h"
#include <QVariantMap>
#include <algorithm>
#include <filesystem>

using namespace utils;
using namespace storage;

namespace {
    Account* findAccount(RegularUser &u, std::string_view num) {
        const std::string key(num);
        for (auto &a : u.accounts) if (a.accountNumber == key) return &a;
        return nullptr;
    }

    bool cancelUserTx(RegularUser &user, std::string_view txId, std::string_view reason,
                      std::string &outToCard, long long &outCents) {
        auto it = std::find_if(user.history.begin(), user.history.end(),
                               [txId](const Transaction& t) { return t.id == txId; });

        if (it == user.history.end()) {
            return false;
        }

        if (it->status == "cancelled") {
            throw ValidationError("Платеж уже отменен");
        }

        if (auto *acc = findAccount(user, it->fromAccount)) {
            acc->balanceCents += it->cents;
        }

        it->status = "cancelled";
        it->cancelReason = std::string(reason);
        user.notifications.push_back("Платеж " + it->id + " отменен: " + std::string(reason));
        outToCard = it->toCard;
        outCents = it->cents;
        UserStorage::saveUser(user);
        return true;
    }

    void tryNotifyRecipient(std::string_view recipientName, std::string_view txId, std::string_view reason) {
        try {
            RegularUser recipient = UserStorage::loadUser(std::string(recipientName));
            recipient.notifications.push_back(
                "Платеж " + std::string(txId) + " отменен администратором. Причина: " + std::string(reason));
            UserStorage::saveUser(recipient);
        } catch (const BankingError &) {
            return;
        }
    }

    namespace {
    Account* findAccountByCard(RegularUser& user, std::string_view cardNum) {
        auto cardIt = std::find_if(user.cards.begin(), user.cards.end(), [cardNum](const Card& card) { return card.cardNumber == cardNum; });
        if (cardIt == user.cards.end()) {
            return nullptr;
        }
        auto accIt = std::find_if(user.accounts.begin(), user.accounts.end(), [&cardIt](const Account& acc) { return acc.accountNumber == cardIt->linkedAccount; });
        return (accIt == user.accounts.end()) ? nullptr : &*accIt;
    }
}

bool adjustRecipientBalance(std::string_view destination, long long deltaCents, std::string *ownerUsername = nullptr) {
    for (const auto &name : UserStorage::listUsernames()) {
        RegularUser u = UserStorage::loadUser(name);
        if (Account *linked = findAccountByCard(u, destination)) {
            linked->balanceCents += deltaCents;
            if (ownerUsername) *ownerUsername = u.usernameValue;
            UserStorage::saveUser(u);
            return true;
        }
    }
    return false;
}
}

AdminController::AdminController(QObject *parent) : QObject(parent) {}

QVariantList AdminController::listAllTransfers(const QString &query) const {
    QVariantList out;
    auto users = UserStorage::loadAll();
    for (const auto &user : users) {
        for (const auto &t : user.history) {
            QVariantMap m;
            m["user"] = QString::fromStdString(user.usernameValue);
            m["id"] = QString::fromStdString(t.id);
            m["fromAccount"] = QString::fromStdString(t.fromAccount);
            m["toCard"] = QString::fromStdString(t.toCard);
            m["cents"] = t.cents;
            m["timestamp"] = static_cast<qint64>(t.timestamp);
            m["note"] = QString::fromStdString(t.note);
            m["status"] = QString::fromStdString(t.status);
            m["cancelReason"] = QString::fromStdString(t.cancelReason);
            if (query.isEmpty() || m["id"].toString().contains(query) || m["user"].toString().contains(query)) {
                out.push_back(m);
            }
        }
    }
    return out;
}

void AdminController::cancelTransfer(const QString &transactionId, const QString &reason) {
    try {
        std::string txId = transactionId.trimmed().toStdString();
        std::string reasonStd = reason.trimmed().toStdString();
        if (txId.empty()) throw ValidationError("Укажите платеж");
        if (reasonStd.empty()) throw ValidationError("Укажите причину отмены");

        for (const auto &name : UserStorage::listUsernames()) {
            RegularUser user = UserStorage::loadUser(name);
            std::string toCard;
            long long cents = 0;
            if (cancelUserTx(user, txId, reasonStd, toCard, cents)) {
                if (std::string recipientName; adjustRecipientBalance(toCard, -cents, &recipientName) && !recipientName.empty() && recipientName != user.usernameValue) {
                    tryNotifyRecipient(recipientName, txId, reasonStd);
                }
                emit infoMessage("Платеж отменен");
                return;
            }
        }
        throw NotFoundError("Платеж не найден");
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void AdminController::clearAllUsers() {
    try {
        if (auto root = storage::usersRoot(); std::filesystem::exists(root)) {
            for (auto &entry : std::filesystem::directory_iterator(root)) {
                std::filesystem::remove_all(entry.path());
            }
        }
        emit infoMessage("Все пользователи удалены");
    } catch (const std::filesystem::filesystem_error &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

QStringList AdminController::listUsers() const {
    QStringList out;
    for (const auto &name : UserStorage::listUsernames()) out.push_back(QString::fromStdString(name));
    return out;
}

QStringList AdminController::searchUsers(const QString &query) const {
    QStringList out;
    auto q = query.trimmed();
    for (const auto &name : UserStorage::listUsernames()) {
        if (QString::fromStdString(name).contains(q)) out.push_back(QString::fromStdString(name));
    }
    return out;
}

QStringList AdminController::sortUsersByAccountCount() const {
    QStringList out;
    auto users = UserStorage::loadAll();
    std::sort(users.begin(), users.end(), [](RegularUser &a, RegularUser &b){
        if (a.accounts.size() == b.accounts.size()) return a.usernameValue < b.usernameValue;
        return a.accounts.size() < b.accounts.size();
    });
    for (const auto &u : users) out << QString::fromStdString(u.usernameValue);
    return out;
}

QStringList AdminController::sortUsers(const QString &sortBy) const {
    QStringList out;
    auto users = UserStorage::loadAll();
    if (auto sortKey = sortBy.trimmed().toLower().toStdString(); sortKey == "accounts" || sortKey == "счета") {
        std::sort(users.begin(), users.end(), [](RegularUser &a, RegularUser &b){
            if (a.accounts.size() == b.accounts.size()) return a.usernameValue < b.usernameValue;
            return a.accounts.size() < b.accounts.size();
        });
    } else if (sortKey == "cards" || sortKey == "карты") {
        std::sort(users.begin(), users.end(), [](RegularUser &a, RegularUser &b){
            if (a.cards.size() == b.cards.size()) return a.usernameValue < b.usernameValue;
            return a.cards.size() < b.cards.size();
        });
    } else if (sortKey == "transactions" || sortKey == "транзакции" || sortKey == "переводы") {
        std::sort(users.begin(), users.end(), [](RegularUser &a, RegularUser &b){
            if (a.history.size() == b.history.size()) return a.usernameValue < b.usernameValue;
            return a.history.size() < b.history.size();
        });
    } else {
        std::sort(users.begin(), users.end(), [](RegularUser &a, RegularUser &b){
            return a.usernameValue < b.usernameValue;
        });
    }
    
    for (const auto &u : users) out << QString::fromStdString(u.usernameValue);
    return out;
}

QVariantList AdminController::getAllUsersInfo(const QString &sortBy) const {
    QVariantList out;
    auto users = UserStorage::loadAll();
    // Sorting logic here...
    for (const auto &u : users) {
        QVariantMap m;
        m["username"] = QString::fromStdString(u.usernameValue);
        m["accountsCount"] = (int)u.accounts.size();
        m["cardsCount"] = (int)u.cards.size();
        m["transactionsCount"] = (int)u.history.size();
        out.append(m);
    }
    return out;
}

QVariantList AdminController::sortTransfers(const QString &sortBy) const {
    QVariantList out;
    // Sorting logic here...
    return out;
}

QVariantList AdminController::listUserCards(const QString &username) {
    QVariantList out;
    try {
        RegularUser user = UserStorage::loadUser(username.toStdString());
        for (const auto &card : user.cards) {
            QVariantMap m;
            m["cardNumber"] = QString::fromStdString(card.cardNumber);
            m["holderName"] = QString::fromStdString(card.holderName);
            m["expiry"] = QString::fromStdString(card.expiry);
            m["linkedAccount"] = QString::fromStdString(card.linkedAccount);
            out.append(m);
        }
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
    return out;
}

QVariantList AdminController::listUserAccounts(const QString &username)
{
    QVariantList out;
    try {
        RegularUser user = UserStorage::loadUser(username.toStdString());
        for (const auto &acc : user.accounts) {
            QVariantMap m;
            m["accountNumber"] = QString::fromStdString(acc.accountNumber);
            m["balanceCents"] = acc.balanceCents;
            m["currency"] = QString::fromStdString(acc.currency);
            out.append(m);
        }
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
    return out;
}

void AdminController::setAccountBalance(const QString &accountNumber, qlonglong cents)
{
    try {
        bool found = false;
        for (const auto &name : UserStorage::listUsernames()) {
            RegularUser user = UserStorage::loadUser(name);
            for (auto &acc : user.accounts) {
                if (acc.accountNumber == accountNumber.toStdString()) {
                    acc.balanceCents = cents;
                    UserStorage::saveUser(user);
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (found) {
            emit infoMessage("Баланс обновлен");
        } else {
            emit errorOccured("Счет не найден");
        }
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}
