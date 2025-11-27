#pragma once

#include <QObject>
#include <QString>
#include <optional>
#include "../models/User.h"

class AuthController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ isAuthenticated NOTIFY authenticatedChanged)
    Q_PROPERTY(bool admin READ isAdmin NOTIFY authenticatedChanged)
    Q_PROPERTY(QString username READ username NOTIFY authenticatedChanged)

public:
    explicit AuthController(QObject *parent = nullptr);

    Q_INVOKABLE void login(const QString &username, const QString &password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void registerUser(const QString &username, const QString &password);

    bool isAuthenticated() const;
    bool isAdmin() const;
    QString username() const;

    RegularUser& getCurrentUser();

signals:
    void authenticatedChanged();
    void errorOccured(const QString &message);
    void infoMessage(const QString &message);

private:
    std::optional<RegularUser> currentUser;
    bool isAdminLogin = false;
};
