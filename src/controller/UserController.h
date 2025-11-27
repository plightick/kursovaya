#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>

class UserController : public QObject {
    Q_OBJECT

public:
    explicit UserController(QObject *parent = nullptr);

    Q_INVOKABLE QStringList listUsers() const;
    Q_INVOKABLE QStringList searchUsers(const QString &query) const;
    Q_INVOKABLE QStringList sortUsersByAccountCount() const;
    Q_INVOKABLE QStringList sortUsers(const QString &sortBy) const;
    Q_INVOKABLE QVariantList getAllUsersInfo(const QString &sortBy = "") const;

signals:
    void errorOccured(const QString &message);
    void infoMessage(const QString &message);
};
