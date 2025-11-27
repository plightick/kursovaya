#pragma once

#include <QObject>
#include <QVariantList>
#include "../models/User.h"

class NotificationController : public QObject {
    Q_OBJECT

public:
    explicit NotificationController(RegularUser &user, QObject *parent = nullptr);

    Q_INVOKABLE QVariantList listNotifications() const;
    Q_INVOKABLE void clearNotifications();

signals:
    void errorOccured(const QString &message);
    void infoMessage(const QString &message);

private:
    RegularUser &currentUser;
    void saveCurrentUser() const;
};
