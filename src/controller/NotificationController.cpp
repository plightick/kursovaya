#include "NotificationController.h"
#include "../storage/UserStorage.h"

using namespace storage;

NotificationController::NotificationController(RegularUser &user, QObject *parent) : QObject(parent), currentUser(user) {}

QVariantList NotificationController::listNotifications() const {
    QVariantList out;
    for (const auto &notification : currentUser.notifications) {
        out.append(QString::fromStdString(notification));
    }
    return out;
}

void NotificationController::clearNotifications() {
    currentUser.notifications.clear();
    saveCurrentUser();
    emit infoMessage("Уведомления очищены");
}

void NotificationController::saveCurrentUser() const {
    UserStorage::saveUser(currentUser);
}
