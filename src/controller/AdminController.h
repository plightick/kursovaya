#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>

class AdminController : public QObject {
    Q_OBJECT

public:
    explicit AdminController(QObject *parent = nullptr);

    Q_INVOKABLE QVariantList listAllTransfers(const QString &query) const;
    Q_INVOKABLE void cancelTransfer(const QString &transactionId, const QString &reason);
    Q_INVOKABLE void clearAllUsers();
    Q_INVOKABLE QStringList listUsers() const;
    Q_INVOKABLE QStringList searchUsers(const QString &query) const;
    Q_INVOKABLE QStringList sortUsersByAccountCount() const;
    Q_INVOKABLE QStringList sortUsers(const QString &sortBy) const; // "accounts", "cards", "transactions", "name"
    Q_INVOKABLE QVariantList getAllUsersInfo(const QString &sortBy = "") const; // Returns full user info with accounts, cards, transactions count
        Q_INVOKABLE QVariantList sortTransfers(const QString &sortBy) const; // "user", "amount", "date", "status"

    Q_INVOKABLE QVariantList listUserCards(const QString &username);
    Q_INVOKABLE QVariantList listUserAccounts(const QString &username);
    Q_INVOKABLE void setAccountBalance(const QString &accountNumber, qlonglong cents);

signals:
    void errorOccured(const QString &message);
    void infoMessage(const QString &message);
};
