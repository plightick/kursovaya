#include "AuthController.h"
#include "../storage/UserStorage.h"
#include "../utils/Exceptions.h"
#include "../utils/Utils.h"

using namespace utils;
using namespace storage;

AuthController::AuthController(QObject *parent) : QObject(parent) {}

void AuthController::login(const QString &username, const QString &password) {
    try {
        auto uname = username.trimmed().toStdString();
        auto pwd = password.toStdString();
        if (uname.empty()) throw ValidationError("Username is empty");

        if (uname == "admin") {
            if (password != "admin") throw AuthError("Неверный пароль администратора");
            isAdminLogin = true;
            currentUser.reset();
            emit authenticatedChanged();
            emit infoMessage("Вход выполнен как администратор");
            return;
        }

        RegularUser u = UserStorage::loadUser(uname);
        if (u.passwordHash != weakHash(pwd)) throw AuthError("Неверный пароль");
        currentUser = std::move(u);
        isAdminLogin = false;
        emit authenticatedChanged();
        emit infoMessage("Вход выполнен");
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void AuthController::logout() {
    currentUser.reset();
    isAdminLogin = false;
    emit authenticatedChanged();
}

void AuthController::registerUser(const QString &username, const QString &password) {
    try {
        auto uname = username.trimmed().toStdString();
        if (uname.empty()) throw ValidationError("Пустое имя пользователя");
        if (UserStorage::exists(uname)) throw ValidationError("Пользователь уже существует");
        RegularUser u(uname, weakHash(password.toStdString()));
        UserStorage::saveUser(u);
        emit infoMessage("Пользователь создан");
    } catch (const BankingError &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

bool AuthController::isAuthenticated() const {
    return currentUser.has_value() || isAdminLogin;
}

bool AuthController::isAdmin() const {
    return isAdminLogin;
}

QString AuthController::username() const {
    if (isAdminLogin) {
        return QStringLiteral("admin");
    }
    if (currentUser) {
        return QString::fromStdString(currentUser->usernameValue);
    }
    return {};
}

RegularUser& AuthController::getCurrentUser() {
    if (!currentUser) throw AuthError("User not authenticated");
    return *currentUser;
}
