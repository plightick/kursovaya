#pragma once

#include <QObject>
#include <QVariantList>
#include "../models/User.h"

class TransactionController : public QObject {
    Q_OBJECT

public:
    explicit TransactionController(RegularUser &user, QObject *parent = nullptr);

    Q_INVOKABLE QVariantList listHistory() const;
    Q_INVOKABLE QVariantList listFavorites() const;
    Q_INVOKABLE void addFavorite(const QString &name, const QString &toCard, const QString &note);
    Q_INVOKABLE void transfer(const QString &fromAccount, const QString &toCard, qlonglong cents, const QString &note, const QString &category = "other");
    Q_INVOKABLE void payFavorite(const QString &favName, const QString &fromAccount, qlonglong cents, const QString &category = "other");
    Q_INVOKABLE QVariantMap getExpenseStats() const;
    Q_INVOKABLE void depositToAccount(const QString &accountNumber, qlonglong cents, const QString &externalAccount);
    Q_INVOKABLE QVariantMap receiptFor(const QString &transactionId) const;
    Q_INVOKABLE QString downloadReceipt(const QString &transactionId);
    Q_INVOKABLE QString saveReceiptToFile(const QString &transactionId, const QString &filePath);

signals:
    void errorOccured(const QString &message);
    void infoMessage(const QString &message);

private:
    RegularUser &currentUser;
    void saveCurrentUser();
    bool adjustRecipientBalance(std::string_view destination, long long deltaCents, std::string *ownerUsername = nullptr) const;
};
