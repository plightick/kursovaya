#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <optional>
#include <unordered_map>
#include <string_view>
#include "../models/User.h"
#include "../models/Account.h"
#include "../models/Card.h"
#include "../models/Transaction.h"
#include "../models/FavoritePayment.h"
#include "../utils/Exceptions.h"
#include "../utils/Utils.h"
#include "../storage/UserStorage.h"

class AuthController;
class AccountController;
class TransactionController;
class AdminController;
class UserController;
class NotificationController;

class BankController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ isAuthenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(bool admin READ isAdmin NOTIFY authenticatedChanged)
    Q_PROPERTY(QString username READ username NOTIFY authenticatedChanged)
public:
    explicit BankController(QObject *parent = nullptr);

    Q_INVOKABLE void seedAdmin() const;

    Q_INVOKABLE void login(const QString &username, const QString &password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void registerUser(const QString &username, const QString &password);

    Q_INVOKABLE QVariantList listAccounts() const;
    Q_INVOKABLE QVariantList listCards() const;
    Q_INVOKABLE QVariantList listHistory() const;
    Q_INVOKABLE QVariantList listFavorites() const;

    Q_INVOKABLE void addAccount(const QString &currency);
    Q_INVOKABLE void addCard([[maybe_unused]] const QString &holderName, const QString &expiry, const QString &linkedAccount);
    Q_INVOKABLE void addFavorite(const QString &name, const QString &toCard, const QString &note);

    Q_INVOKABLE void transfer(const QString &fromAccount, const QString &toCard, qlonglong cents, const QString &note, const QString &category = "other");
    Q_INVOKABLE void payFavorite(const QString &favName, const QString &fromAccount, qlonglong cents, const QString &category = "other");
    Q_INVOKABLE QVariantMap getExpenseStats() const;
    Q_INVOKABLE void depositToAccount(const QString &accountNumber, qlonglong cents, const QString &externalAccount);
    Q_INVOKABLE QVariantMap receiptFor(const QString &transactionId) const;
    Q_INVOKABLE QString downloadReceipt(const QString &transactionId);
    Q_INVOKABLE QString saveReceiptToFile(const QString &transactionId, const QString &filePath);


    Q_INVOKABLE void setAccountBalance(const QString &accountNumber, qlonglong cents);

    Q_INVOKABLE QVariantList listAllTransfers(const QString &query) const;
    Q_INVOKABLE void cancelTransfer(const QString &transactionId, const QString &reason);
    Q_INVOKABLE void clearAllUsers();

    Q_INVOKABLE QVariantList sortTransfers(const QString &sortBy) const; // "user", "amount", "date", "status"

    Q_INVOKABLE QString ratesText() const;
    Q_INVOKABLE bool isCardExpired(const QString &expiry) const;

    bool isAuthenticated() const;
    bool isAdmin() const;
    QString username() const;

signals:
    void authenticatedChanged();
    void errorOccured(const QString &message);
    void infoMessage(const QString &message);

private slots:
    void onAuthenticatedChanged();

private:
    AuthController *m_authController = nullptr;
    AccountController *m_accountController = nullptr;
    TransactionController *m_transactionController = nullptr;
    AdminController *m_adminController = nullptr;
    UserController *m_userController = nullptr;
    NotificationController *m_notificationController = nullptr;
};
