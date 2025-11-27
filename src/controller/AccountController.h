#pragma once

#include <QObject>
#include <QVariantList>
#include "../models/User.h"

class AccountController : public QObject {
    Q_OBJECT

public:
    explicit AccountController(RegularUser &user, QObject *parent = nullptr);

    Q_INVOKABLE QVariantList listAccounts() const;
    Q_INVOKABLE QVariantList listCards() const;
    Q_INVOKABLE void addAccount(const QString &currency);
    Q_INVOKABLE void addCard(const QString &holderName, const QString &expiry, const QString &linkedAccount);

signals:
    void errorOccured(const QString &message);
    void infoMessage(const QString &message);

private:
    RegularUser &currentUser;
    void saveCurrentUser() const;
};
