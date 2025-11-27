#include "TransactionController.h"
#include "../storage/UserStorage.h"
#include "../utils/Exceptions.h"
#include "../utils/Utils.h"
#include <QVariantMap>
#include <QDateTime>
#include <filesystem>
#include <fstream>
#include <chrono>

using namespace utils;
using namespace storage;

TransactionController::TransactionController(RegularUser &user, QObject *parent) : QObject(parent), currentUser(user) {}

QVariantList TransactionController::listHistory() const {
    QVariantList out;
    for (const auto &t : currentUser.history) {
        QVariantMap m;
        m["id"] = QString::fromStdString(t.id);
        m["fromAccount"] = QString::fromStdString(t.fromAccount);
        m["toCard"] = QString::fromStdString(t.toCard);
        m["cents"] = t.cents;
        m["timestamp"] = static_cast<qint64>(t.timestamp);
        m["note"] = QString::fromStdString(t.note);
        m["status"] = QString::fromStdString(t.status);
        m["cancelReason"] = QString::fromStdString(t.cancelReason);
        out.push_back(m);
    }
    return out;
}

QVariantList TransactionController::listFavorites() const {
    QVariantList out;
    for (const auto &f : currentUser.favorites) {
        QVariantMap m;
        m["name"] = QString::fromStdString(f.name);
        m["toCard"] = QString::fromStdString(f.toCard);
        m["note"] = QString::fromStdString(f.note);
        out.push_back(m);
    }
    return out;
}

void TransactionController::addFavorite(const QString &name, const QString &toCard, const QString &note) {
    try {
        FavoritePayment f;
        f.name = name.toStdString();
        f.toCard = toCard.toStdString();
        f.note = note.toStdString();
        currentUser.favorites.push_back(f);
        saveCurrentUser();
        emit infoMessage("Избранный платеж добавлен");
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void TransactionController::transfer(const QString &fromAccount, const QString &toCard, qlonglong cents, const QString &note, const QString &category) {
    try {
        Account *src = nullptr;
        const std::string fromId = fromAccount.toStdString();
        for (auto &a : currentUser.accounts) { if (a.accountNumber == fromId) { src = &a; break; } }
        if (!src) throw ValidationError("Нет такого счета");
        if (cents <= 0) throw ValidationError("Сумма должна быть положительной");
        if (src->balanceCents < cents) throw ValidationError("Недостаточно средств");
        src->balanceCents -= cents;

        Transaction t;
        t.id = generateNumericId(12);
        t.fromAccount = fromAccount.toStdString();
        t.toCard = toCard.toStdString();
        t.cents = cents;
        t.timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        t.note = note.toStdString();
        t.category = category.isEmpty() ? "other" : category.toStdString();
        t.status = "completed";
        t.cancelReason.clear();
        currentUser.history.push_back(t);
        saveCurrentUser();

        bool credited = adjustRecipientBalance(toCard.toStdString(), cents);

        emit infoMessage(credited ? "Перевод выполнен" : "Перевод выполнен (получатель не найден)");
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void TransactionController::payFavorite(const QString &favName, const QString &fromAccount, qlonglong cents, const QString &category) {
    try {
        const std::string wanted = favName.toStdString();
        const FavoritePayment *pf = nullptr;
        for (const auto &f : currentUser.favorites) { if (f.name == wanted) { pf = &f; break; } }
        if (!pf) throw ValidationError("Нет такого избранного платежа");
        transfer(fromAccount, QString::fromStdString(pf->toCard), cents, QString::fromStdString(pf->note), category);
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void TransactionController::depositToAccount(const QString &accountNumber, qlonglong cents, const QString &externalAccount) {
    try {
        if (cents <= 0) throw ValidationError("Сумма должна быть положительной");
        auto accountId = accountNumber.toStdString();
        Account *dst = nullptr;
        for (auto &a : currentUser.accounts) { if (a.accountNumber == accountId) { dst = &a; break; } }
        if (!dst) throw ValidationError("Нет такого счета");
        dst->balanceCents += cents;

        Transaction t;
        t.id = generateNumericId(12);
        t.fromAccount = externalAccount.toStdString();
        t.toCard = accountId;
        t.cents = cents;
        t.timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        t.note = "Пополнение счета";
        t.category = "other";
        t.status = "completed";
        currentUser.history.push_back(t);
        saveCurrentUser();
        emit infoMessage("Счет пополнен");
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

QVariantMap TransactionController::getExpenseStats() const {
    QVariantMap stats;
    std::unordered_map<std::string, long long, utils::TransparentStringHash, std::equal_to<>> categoryTotals;
    for (const auto &t : currentUser.history) {
        if (!(t.cents > 0 && t.status == "completed")) continue;
        bool isOutgoing = false;
        for (const auto &acc : currentUser.accounts) {
            if (acc.accountNumber == t.fromAccount) {
                isOutgoing = true;
                break;
            }
        }
        if (!isOutgoing) continue;
        std::string cat = t.category.empty() ? "other" : t.category;
        categoryTotals[cat] += t.cents;
    }

    QVariantMap result;
    long long total = 0;
    for (const auto &[cat, amount] : categoryTotals) {
        total += amount;
    }

    for (const auto &[cat, amount] : categoryTotals) {
        QVariantMap catData;
        catData["name"] = QString::fromStdString(cat);
        catData["amount"] = amount;
        catData["percent"] = total > 0 ? (amount * 100.0 / total) : 0.0;
        result[QString::fromStdString(cat)] = catData;
    }
    result["total"] = total;
    return result;
}

QVariantMap TransactionController::receiptFor(const QString &transactionId) const {
    QVariantMap out;
    std::string txId = transactionId.trimmed().toStdString();
    for (const auto &t : currentUser.history) {
        if (t.id == txId) {
            out["user"] = QString::fromStdString(currentUser.usernameValue);
            out["id"] = QString::fromStdString(t.id);
            out["fromAccount"] = QString::fromStdString(t.fromAccount);
            out["toCard"] = QString::fromStdString(t.toCard);
            out["cents"] = t.cents;
            out["timestamp"] = static_cast<qint64>(t.timestamp);
            out["note"] = QString::fromStdString(t.note);
            out["status"] = QString::fromStdString(t.status);
            out["cancelReason"] = QString::fromStdString(t.cancelReason);
            break;
        }
    }
    return out;
}

QString TransactionController::downloadReceipt(const QString &transactionId) {
    std::filesystem::create_directories("data/receipts");
    auto filename = "data/receipts/receipt_" + transactionId.trimmed().toStdString() + ".txt";
    return saveReceiptToFile(transactionId, QString::fromStdString(filename));
}

QString TransactionController::saveReceiptToFile(const QString &transactionId, const QString &filePath) {
    try {
        auto receipt = receiptFor(transactionId);
        if (receipt.isEmpty()) {
            emit errorOccured("Чек не найден");
            return QString();
        }
        
        std::ofstream ofs(filePath.toStdString());
        if (!ofs) {
            emit errorOccured("Ошибка при создании файла");
            return QString();
        }
        
        ofs << "ЧЕК О ПЕРЕВОДЕ\n";
        ofs << "================\n";
        ofs << "ID транзакции: " << receipt["id"].toString().toStdString() << "\n";
        ofs << "Пользователь: " << receipt["user"].toString().toStdString() << "\n";
        ofs << "Отправитель: " << receipt["fromAccount"].toString().toStdString() << "\n";
        ofs << "Получатель: " << receipt["toCard"].toString().toStdString() << "\n";
        ofs << "Сумма: " << (receipt["cents"].toLongLong() / 100.0) << "\n";
        ofs << "Статус: " << receipt["status"].toString().toStdString() << "\n";
        ofs << "Примечание: " << receipt["note"].toString().toStdString() << "\n";
        if (!receipt["cancelReason"].toString().isEmpty()) {
            ofs << "Причина отмены: " << receipt["cancelReason"].toString().toStdString() << "\n";
        }
        if (receipt["timestamp"].toLongLong() > 0) {
            qint64 ts = receipt["timestamp"].toLongLong();
            QString dt = QDateTime::fromSecsSinceEpoch(ts).toString("yyyy-MM-dd HH:mm:ss");
            ofs << "Дата/время: " << dt.toStdString() << "\n";
        }
        ofs << "================\n";
        emit infoMessage("Чек сохранен: " + filePath);
        return filePath;
    } catch (const std::ios_base::failure &e) {
        emit errorOccured(QString::fromStdString(e.what()));
        return QString();
    }
}

void TransactionController::saveCurrentUser() {
    UserStorage::saveUser(currentUser);
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

bool TransactionController::adjustRecipientBalance(std::string_view destination, long long deltaCents, std::string *ownerUsername) const {
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
