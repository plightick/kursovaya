#include "UserController.h"
#include "../storage/UserStorage.h"
#include "../utils/Exceptions.h"
#include <algorithm>

using namespace storage;

UserController::UserController(QObject *parent) : QObject(parent) {}

QStringList UserController::listUsers() const {
    QStringList out;
    for (const auto &name : UserStorage::listUsernames()) out.push_back(QString::fromStdString(name));
    return out;
}

QStringList UserController::searchUsers(const QString &query) const {
    QStringList out;
    auto q = query.trimmed();
    for (const auto &name : UserStorage::listUsernames()) {
        if (QString::fromStdString(name).contains(q)) out.push_back(QString::fromStdString(name));
    }
    return out;
}

QStringList UserController::sortUsersByAccountCount() const {
    QStringList out;
    auto users = UserStorage::loadAll();
    std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
        if (a.accounts.size() == b.accounts.size()) return a.usernameValue < b.usernameValue;
        return a.accounts.size() < b.accounts.size();
    });
    for (const auto &u : users) out << QString::fromStdString(u.usernameValue);
    return out;
}

QStringList UserController::sortUsers(const QString &sortBy) const {
    QStringList out;
    auto users = UserStorage::loadAll();
    if (auto sortKey = sortBy.trimmed().toLower().toStdString(); sortKey == "accounts" || sortKey == "счета") {
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            if (a.accounts.size() == b.accounts.size()) return a.usernameValue < b.usernameValue;
            return a.accounts.size() < b.accounts.size();
        });
    } else if (sortKey == "cards" || sortKey == "карты") {
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            if (a.cards.size() == b.cards.size()) return a.usernameValue < b.usernameValue;
            return a.cards.size() < b.cards.size();
        });
    } else if (sortKey == "transactions" || sortKey == "транзакции" || sortKey == "переводы") {
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            if (a.history.size() == b.history.size()) return a.usernameValue < b.usernameValue;
            return a.history.size() < b.history.size();
        });
    } else {
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            return a.usernameValue < b.usernameValue;
        });
    }
    for (const auto &u : users) out << QString::fromStdString(u.usernameValue);
    return out;
}

QVariantList UserController::getAllUsersInfo(const QString &sortBy) const {
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
