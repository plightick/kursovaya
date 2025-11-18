#include "BankController.h"

#include <QVariantMap>
#include <QDateTime>
#include <QDate>
#include <filesystem>
#include <fstream>

using namespace utils;
using namespace storage;

BankController::BankController(QObject *parent) : QObject(parent) {
}

void BankController::seedAdmin() {
    UserStorage::ensureDataDirs();
}

void BankController::login(const QString &username, const QString &password) {
    try {
        auto uname = username.trimmed().toStdString();
        auto pwd = password.toStdString();
        if (uname.empty()) throw ValidationError("Username is empty");

        if (uname == "admin") {
            if (password == "admin") {
                isAdminLogin = true;
                currentUser.reset();
                emit authenticatedChanged();
                emit infoMessage("Вход выполнен как администратор");
                return;
            } else {
                throw AuthError("Неверный пароль администратора");
            }
        }

        RegularUser u = UserStorage::loadUser(uname);
        if (u.passwordHash != weakHash(pwd)) throw AuthError("Неверный пароль");
        currentUser = std::move(u);
        isAdminLogin = false;
        emit authenticatedChanged();
        emit infoMessage("Вход выполнен");
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void BankController::logout() {
    currentUser.reset();
    isAdminLogin = false;
    emit authenticatedChanged();
}

void BankController::registerUser(const QString &username, const QString &password) {
    try {
        auto uname = username.trimmed().toStdString();
        if (uname.empty()) throw ValidationError("Пустое имя пользователя");
        if (UserStorage::exists(uname)) throw ValidationError("Пользователь уже существует");
        RegularUser u(uname, weakHash(password.toStdString()));
        UserStorage::saveUser(u);
        emit infoMessage("Пользователь создан");
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

QVariantList BankController::listAccounts() const {
    QVariantList out;
    if (!currentUser) return out;
    for (const auto &a : currentUser->accounts) {
        QVariantMap m;
        m["accountNumber"] = QString::fromStdString(a.accountNumber);
        m["currency"] = QString::fromStdString(a.currency);
        m["balanceCents"] = static_cast<qlonglong>(a.balanceCents);
        out.push_back(m);
    }
    return out;
}

QVariantList BankController::listCards() const {
    QVariantList out;
    if (!currentUser) return out;
    for (const auto &c : currentUser->cards) {
        QVariantMap m;
        m["cardNumber"] = QString::fromStdString(c.cardNumber);
        m["holderName"] = QString::fromStdString(c.holderName);
        m["expiry"] = QString::fromStdString(c.expiry);
        m["linkedAccount"] = QString::fromStdString(c.linkedAccount);
        out.push_back(m);
    }
    return out;
}

QVariantList BankController::listUserCards(const QString &username) const {
    QVariantList out;
    try {
        auto uname = username.trimmed().toStdString();
        if (uname.empty()) return out;
        RegularUser u = storage::UserStorage::loadUser(uname);
        for (const auto &c : u.cards) {
            QVariantMap m;
            m["cardNumber"] = QString::fromStdString(c.cardNumber);
            m["holderName"] = QString::fromStdString(c.holderName);
            m["expiry"] = QString::fromStdString(c.expiry);
            m["linkedAccount"] = QString::fromStdString(c.linkedAccount);
            out.push_back(m);
        }
    } catch (...) {
        // ignore missing user
    }
    return out;
}

QVariantList BankController::listUserAccounts(const QString &username) const {
    QVariantList out;
    try {
        auto uname = username.trimmed().toStdString();
        if (uname.empty()) return out;
        RegularUser u = storage::UserStorage::loadUser(uname);
        for (const auto &a : u.accounts) {
            QVariantMap m;
            m["accountNumber"] = QString::fromStdString(a.accountNumber);
            m["currency"] = QString::fromStdString(a.currency);
            m["balanceCents"] = static_cast<qlonglong>(a.balanceCents);
            out.push_back(m);
        }
    } catch (...) {
    }
    return out;
}

QVariantList BankController::listHistory() const {
    QVariantList out;
    if (!currentUser) return out;
    for (const auto &t : currentUser->history) {
        QVariantMap m;
        m["id"] = QString::fromStdString(t.id);
        m["fromAccount"] = QString::fromStdString(t.fromAccount);
        m["toCard"] = QString::fromStdString(t.toCard);
        m["cents"] = static_cast<qlonglong>(t.cents);
        m["timestamp"] = static_cast<qlonglong>(t.timestamp);
        m["note"] = QString::fromStdString(t.note);
        m["status"] = QString::fromStdString(t.status);
        m["cancelReason"] = QString::fromStdString(t.cancelReason);
        out.push_back(m);
    }
    return out;
}

QVariantList BankController::listFavorites() const {
    QVariantList out;
    if (!currentUser) return out;
    for (const auto &f : currentUser->favorites) {
        QVariantMap m;
        m["name"] = QString::fromStdString(f.name);
        m["toCard"] = QString::fromStdString(f.toCard);
        m["note"] = QString::fromStdString(f.note);
        out.push_back(m);
    }
    return out;
}

void BankController::addAccount(const QString &currency) {
    try {
        if (!currentUser) throw AuthError("Необходима авторизация");
        Account a;
        a.accountNumber = generateNumericId(20);
        a.currency = currency.toStdString();
        a.balanceCents = 0;
        currentUser->accounts.push_back(a);
        saveCurrent();
        emit infoMessage("Счет добавлен");
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void BankController::addCard(const QString &holderName, const QString &expiry, const QString &linkedAccount) {
    Q_UNUSED(holderName);  // Имя берется из текущего пользователя
    try {
        if (!currentUser) throw AuthError("Необходима авторизация");
        auto it = std::find_if(currentUser->accounts.begin(), currentUser->accounts.end(), [&](const Account &a){
            return a.accountNumber == linkedAccount.toStdString();
        });
        if (it == currentUser->accounts.end()) throw ValidationError("Нет такого счета");
        Card c;
        c.cardNumber = generateNumericId(16);
        c.holderName = currentUser->usernameValue;
        c.expiry = expiry.toStdString();
        c.linkedAccount = linkedAccount.toStdString();
        currentUser->cards.push_back(c);
        saveCurrent();
        emit infoMessage("Карта добавлена");
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void BankController::addFavorite(const QString &name, const QString &toCard, const QString &note) {
    try {
        if (!currentUser) throw AuthError("Необходима авторизация");
        FavoritePayment f;
        f.name = name.toStdString();
        f.toCard = toCard.toStdString();
        f.note = note.toStdString();
        currentUser->favorites.push_back(f);
        saveCurrent();
        emit infoMessage("Избранный платеж добавлен");
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void BankController::transfer(const QString &fromAccount, const QString &toCard, qlonglong cents, const QString &note, const QString &category) {
    try {
        if (!currentUser) throw AuthError("Необходима авторизация");
        auto it = std::find_if(currentUser->accounts.begin(), currentUser->accounts.end(), [&](const Account &a){
            return a.accountNumber == fromAccount.toStdString();
        });
        if (it == currentUser->accounts.end()) throw ValidationError("Нет такого счета");
        if (cents <= 0) throw ValidationError("Сумма должна быть положительной");
        if (it->balanceCents < cents) throw ValidationError("Недостаточно средств");
        it->balanceCents -= cents;

        Transaction t;
        t.id = generateNumericId(12);
        t.fromAccount = fromAccount.toStdString();
        t.toCard = toCard.toStdString();
        t.cents = cents;
        t.timestamp = std::time(nullptr);
        t.note = note.toStdString();
        t.category = category.isEmpty() ? "other" : category.toStdString();
        t.status = "completed";
        t.cancelReason.clear();
        currentUser->history.push_back(t);
        saveCurrent();

        bool credited = adjustRecipientBalance(toCard.toStdString(), cents);

        emit infoMessage(credited ? "Перевод выполнен" : "Перевод выполнен (получатель не найден)");
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void BankController::payFavorite(const QString &favName, const QString &fromAccount, qlonglong cents, const QString &category) {
    try {
        if (!currentUser) throw AuthError("Необходима авторизация");
        auto it = std::find_if(currentUser->favorites.begin(), currentUser->favorites.end(), [&](const FavoritePayment &f){
            return f.name == favName.toStdString();
        });
        if (it == currentUser->favorites.end()) throw ValidationError("Нет такого избранного платежа");
        transfer(fromAccount, QString::fromStdString(it->toCard), cents, QString::fromStdString(it->note), category);
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void BankController::depositToAccount(const QString &accountNumber, qlonglong cents, const QString &externalAccount) {
    try {
        if (!currentUser) throw AuthError("Необходима авторизация");
        if (cents <= 0) throw ValidationError("Сумма должна быть положительной");
        auto accountId = accountNumber.toStdString();
        auto it = std::find_if(currentUser->accounts.begin(), currentUser->accounts.end(), [&](const Account &a){
            return a.accountNumber == accountId;
        });
        if (it == currentUser->accounts.end()) throw ValidationError("Нет такого счета");
        it->balanceCents += cents;

        Transaction t;
        t.id = generateNumericId(12);
        t.fromAccount = externalAccount.toStdString();
        t.toCard = accountId;
        t.cents = cents;
        t.timestamp = std::time(nullptr);
        t.note = "Пополнение счета";
        t.category = "other";
        t.status = "completed";
        currentUser->history.push_back(t);
        saveCurrent();
        emit infoMessage("Счет пополнен");
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void BankController::setAccountBalance(const QString &accountNumber, qlonglong cents) {
    Q_UNUSED(accountNumber);
    Q_UNUSED(cents);
    emit errorOccured("Используйте кнопку пополнения");
}

QStringList BankController::listUsers() const {
    QStringList out;
    if (!isAdminLogin) return out;
    for (const auto &name : UserStorage::listUsernames()) out.push_back(QString::fromStdString(name));
    return out;
}

QStringList BankController::searchUsers(const QString &query) const {
    QStringList out;
    if (!isAdminLogin) return out;
    auto q = query.trimmed().toStdString();
    for (const auto &name : UserStorage::listUsernames()) {
        if (name.find(q) != std::string::npos) out.push_back(QString::fromStdString(name));
    }
    return out;
}

QStringList BankController::sortUsersByAccountCount() const {
    QStringList out;
    if (!isAdminLogin) return out;
    auto users = UserStorage::loadAll();
    std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
        if (a.accounts.size() == b.accounts.size()) return a.usernameValue < b.usernameValue;
        return a.accounts.size() < b.accounts.size();
    });
    for (const auto &u : users) out << QString::fromStdString(u.usernameValue);
    return out;
}

QStringList BankController::sortUsers(const QString &sortBy) const {
    QStringList out;
    if (!isAdminLogin) return out;
    auto users = UserStorage::loadAll();
    std::string sort = sortBy.trimmed().toLower().toStdString();
    
    if (sort == "accounts" || sort == "счета") {
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            if (a.accounts.size() == b.accounts.size()) return a.usernameValue < b.usernameValue;
            return a.accounts.size() < b.accounts.size();
        });
    } else if (sort == "cards" || sort == "карты") {
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            if (a.cards.size() == b.cards.size()) return a.usernameValue < b.usernameValue;
            return a.cards.size() < b.cards.size();
        });
    } else if (sort == "transactions" || sort == "транзакции" || sort == "переводы") {
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            if (a.history.size() == b.history.size()) return a.usernameValue < b.usernameValue;
            return a.history.size() < b.history.size();
        });
    } else {
        // По умолчанию сортировка по имени
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            return a.usernameValue < b.usernameValue;
        });
    }
    
    for (const auto &u : users) out << QString::fromStdString(u.usernameValue);
    return out;
}

QVariantList BankController::getAllUsersInfo(const QString &sortBy) const {
    QVariantList out;
    if (!isAdminLogin) return out;
    
    auto users = UserStorage::loadAll();
    std::string sort = sortBy.trimmed().toLower().toStdString();
    
    // Сортировка
    if (sort == "accounts" || sort == "счета") {
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            if (a.accounts.size() == b.accounts.size()) return a.usernameValue < b.usernameValue;
            return a.accounts.size() < b.accounts.size();
        });
    } else if (sort == "cards" || sort == "карты") {
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            if (a.cards.size() == b.cards.size()) return a.usernameValue < b.usernameValue;
            return a.cards.size() < b.cards.size();
        });
    } else if (sort == "transactions" || sort == "транзакции" || sort == "переводы") {
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            if (a.history.size() == b.history.size()) return a.usernameValue < b.usernameValue;
            return a.history.size() < b.history.size();
        });
    } else {
        // По умолчанию сортировка по имени
        std::sort(users.begin(), users.end(), [](const RegularUser &a, const RegularUser &b){
            return a.usernameValue < b.usernameValue;
        });
    }
    
    // Формируем список с полной информацией
    for (const auto &u : users) {
        QVariantMap m;
        m["username"] = QString::fromStdString(u.usernameValue);
        m["accountsCount"] = static_cast<int>(u.accounts.size());
        m["cardsCount"] = static_cast<int>(u.cards.size());
        m["transactionsCount"] = static_cast<int>(u.history.size());
        m["favoritesCount"] = static_cast<int>(u.favorites.size());
        m["notificationsCount"] = static_cast<int>(u.notifications.size());
        
        // Подсчитываем общий баланс
        long long totalBalance = 0;
        for (const auto &acc : u.accounts) {
            totalBalance += acc.balanceCents;
        }
        m["totalBalance"] = static_cast<qlonglong>(totalBalance);
        
        // Список счетов
        QVariantList accountsList;
        for (const auto &acc : u.accounts) {
            QVariantMap accMap;
            accMap["accountNumber"] = QString::fromStdString(acc.accountNumber);
            accMap["currency"] = QString::fromStdString(acc.currency);
            accMap["balanceCents"] = static_cast<qlonglong>(acc.balanceCents);
            accountsList.append(accMap);
        }
        m["accounts"] = accountsList;
        
        // Список карт
        QVariantList cardsList;
        for (const auto &card : u.cards) {
            QVariantMap cardMap;
            cardMap["cardNumber"] = QString::fromStdString(card.cardNumber);
            cardMap["holderName"] = QString::fromStdString(card.holderName);
            cardMap["expiry"] = QString::fromStdString(card.expiry);
            cardMap["linkedAccount"] = QString::fromStdString(card.linkedAccount);
            cardsList.append(cardMap);
        }
        m["cards"] = cardsList;
        
        out.append(m);
    }
    
    return out;
}

QVariantList BankController::sortTransfers(const QString &sortBy) const {
    QVariantList out;
    if (!isAdminLogin) return out;
    
    std::string sort = sortBy.trimmed().toLower().toStdString();
    auto users = UserStorage::loadAll();
    std::vector<QVariantMap> transfers;
    
    for (const auto &u : users) {
        for (const auto &t : u.history) {
            QVariantMap m;
            m["user"] = QString::fromStdString(u.usernameValue);
            m["id"] = QString::fromStdString(t.id);
            m["fromAccount"] = QString::fromStdString(t.fromAccount);
            m["toCard"] = QString::fromStdString(t.toCard);
            m["cents"] = static_cast<qlonglong>(t.cents);
            m["timestamp"] = static_cast<qlonglong>(t.timestamp);
            m["note"] = QString::fromStdString(t.note);
            m["status"] = QString::fromStdString(t.status);
            m["cancelReason"] = QString::fromStdString(t.cancelReason);
            transfers.push_back(m);
        }
    }
    
    if (sort == "user" || sort == "пользователь") {
        std::sort(transfers.begin(), transfers.end(), [](const QVariantMap &a, const QVariantMap &b){
            QString userA = a["user"].toString();
            QString userB = b["user"].toString();
            if (userA == userB) {
                return a["timestamp"].toLongLong() > b["timestamp"].toLongLong();
            }
            return userA < userB;
        });
    } else if (sort == "amount" || sort == "сумма") {
        std::sort(transfers.begin(), transfers.end(), [](const QVariantMap &a, const QVariantMap &b){
            qlonglong amountA = a["cents"].toLongLong();
            qlonglong amountB = b["cents"].toLongLong();
            if (amountA == amountB) {
                return a["timestamp"].toLongLong() > b["timestamp"].toLongLong();
            }
            return amountA > amountB;
        });
    } else if (sort == "date" || sort == "дата") {
        std::sort(transfers.begin(), transfers.end(), [](const QVariantMap &a, const QVariantMap &b){
            return a["timestamp"].toLongLong() > b["timestamp"].toLongLong();
        });
    } else if (sort == "status" || sort == "статус") {
        std::sort(transfers.begin(), transfers.end(), [](const QVariantMap &a, const QVariantMap &b){
            QString statusA = a["status"].toString();
            QString statusB = b["status"].toString();
            if (statusA == statusB) {
                return a["timestamp"].toLongLong() > b["timestamp"].toLongLong();
            }
            return statusA < statusB;
        });
    }
    
    for (const auto &m : transfers) out.push_back(m);
    return out;
}

QVariantMap BankController::receiptFor(const QString &transactionId) const {
    QVariantMap out;
    std::string txId = transactionId.trimmed().toStdString();
    auto build = [&](const RegularUser &u, const Transaction &t) {
        QVariantMap m;
        m["user"] = QString::fromStdString(u.usernameValue);
        m["id"] = QString::fromStdString(t.id);
        m["fromAccount"] = QString::fromStdString(t.fromAccount);
        m["toCard"] = QString::fromStdString(t.toCard);
        m["cents"] = static_cast<qlonglong>(t.cents);
        m["timestamp"] = static_cast<qlonglong>(t.timestamp);
        m["note"] = QString::fromStdString(t.note);
        m["status"] = QString::fromStdString(t.status);
        m["cancelReason"] = QString::fromStdString(t.cancelReason);
        return m;
    };

    if (isAdminLogin) {
        for (const auto &user : UserStorage::loadAll()) {
            auto it = std::find_if(user.history.begin(), user.history.end(), [&](const Transaction &t){ return t.id == txId; });
            if (it != user.history.end()) {
                return build(user, *it);
            }
        }
    } else if (currentUser) {
        auto it = std::find_if(currentUser->history.begin(), currentUser->history.end(), [&](const Transaction &t){ return t.id == txId; });
        if (it != currentUser->history.end()) {
            return build(*currentUser, *it);
        }
    }
    return out;
}

QString BankController::downloadReceipt(const QString &transactionId) {
    // Старый метод для обратной совместимости - сохраняет в data/receipts
    std::filesystem::create_directories("data/receipts");
    auto filename = "data/receipts/receipt_" + transactionId.trimmed().toStdString() + ".txt";
    return saveReceiptToFile(transactionId, QString::fromStdString(filename));
}

QString BankController::saveReceiptToFile(const QString &transactionId, const QString &filePath) {
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
            auto ts = static_cast<std::time_t>(receipt["timestamp"].toLongLong());
            char buf[100];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&ts));
            ofs << "Дата/время: " << buf << "\n";
        }
        ofs << "================\n";
        emit infoMessage("Чек сохранен: " + filePath);
        return filePath;
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
        return QString();
    } catch (...) {
        emit errorOccured("Неизвестная ошибка при сохранении чека");
        return QString();
    }
}

QVariantMap BankController::getExpenseStats() const {
    QVariantMap stats;
    if (!currentUser) return stats;
    
    std::unordered_map<std::string, long long> categoryTotals;
    for (const auto &t : currentUser->history) {
        if (t.cents > 0 && t.status == "completed") {
            // Учитываем только исходящие переводы (не пополнения)
            // Пополнения имеют fromAccount как внешний счет, а toCard как наш счет
            bool isOutgoing = false;
            for (const auto &acc : currentUser->accounts) {
                if (acc.accountNumber == t.fromAccount) {
                    isOutgoing = true;
                    break;
                }
            }
            if (isOutgoing) {
                categoryTotals[t.category.empty() ? "other" : t.category] += t.cents;
            }
        }
    }
    
    // Преобразуем в QVariantMap с русскими названиями
    std::map<std::string, QString> categoryNames = {
        {"medicine", "Медицина и здравоохранение"},
        {"sport", "Спорт"},
        {"food", "Продукты"},
        {"entertainment", "Развлечения"},
        {"other", "Остальное"}
    };
    
    long long total = 0;
    for (const auto &[cat, amount] : categoryTotals) {
        total += amount;
    }
    
    QVariantMap result;
    for (const auto &[key, name] : categoryNames) {
        long long amount = categoryTotals.count(key) ? categoryTotals[key] : 0;
        QVariantMap catData;
        catData["name"] = name;
        catData["amount"] = static_cast<qlonglong>(amount);
        catData["percent"] = total > 0 ? (amount * 100.0 / total) : 0.0;
        result[QString::fromStdString(key)] = catData;
    }
    result["total"] = static_cast<qlonglong>(total);
    return result;
}

QVariantList BankController::listNotifications() const {
    QVariantList out;
    if (!currentUser) return out;
    for (const auto &n : currentUser->notifications) {
        QVariantMap m;
        m["message"] = QString::fromStdString(n);
        out.push_back(m);
    }
    return out;
}

void BankController::clearNotifications() {
    try {
        if (!currentUser) throw AuthError("Необходима авторизация");
        currentUser->notifications.clear();
        saveCurrent();
        emit infoMessage("Уведомления очищены");
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void BankController::cancelTransfer(const QString &transactionId, const QString &reason) {
    try {
        if (!isAdminLogin) throw AuthError("Только администратор может отменять платежи");
        std::string txId = transactionId.trimmed().toStdString();
        std::string reasonStd = reason.trimmed().toStdString();
        if (txId.empty()) throw ValidationError("Укажите платеж");
        if (reasonStd.empty()) throw ValidationError("Укажите причину отмены");

        bool found = false;
        for (const auto &name : UserStorage::listUsernames()) {
            RegularUser user = UserStorage::loadUser(name);
            auto it = std::find_if(user.history.begin(), user.history.end(), [&](const Transaction &t){ return t.id == txId; });
            if (it != user.history.end()) {
                if (it->status == "cancelled") throw ValidationError("Платеж уже отменен");
                auto accIt = std::find_if(user.accounts.begin(), user.accounts.end(), [&](const Account &a){ return a.accountNumber == it->fromAccount; });
                if (accIt != user.accounts.end()) accIt->balanceCents += it->cents;
                it->status = "cancelled";
                it->cancelReason = reasonStd;
                user.notifications.push_back("Платеж " + it->id + " отменен: " + reasonStd);
                UserStorage::saveUser(user);

                // снять деньги у получателя
                std::string recipientName;
                if (adjustRecipientBalance(it->toCard, -it->cents, &recipientName) && !recipientName.empty() && recipientName != user.usernameValue) {
                    try {
                        RegularUser recipient = UserStorage::loadUser(recipientName);
                        recipient.notifications.push_back("Платеж " + it->id + " отменен администратором. Причина: " + reasonStd);
                        UserStorage::saveUser(recipient);
                    } catch (...) {}
                }
                found = true;
                break;
            }
        }
        if (!found) throw NotFoundError("Платеж не найден");
        emit infoMessage("Платеж отменен");
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void BankController::clearAllUsers() {
    try {
        if (!isAdminLogin) throw AuthError("Только администратор");
        auto root = storage::usersRoot();
        if (std::filesystem::exists(root)) {
            for (auto &entry : std::filesystem::directory_iterator(root)) {
                std::filesystem::remove_all(entry.path());
            }
        }
        emit infoMessage("Все пользователи удалены");
    } catch (const std::exception &e) {
        emit errorOccured(QString::fromStdString(e.what()));
    }
}

void BankController::saveCurrent() {
    if (currentUser) {
        UserStorage::saveUser(*currentUser);
    }
}

QString BankController::ratesText() const {
    try {
        std::filesystem::create_directories("data");
        auto path = std::filesystem::path("data/rates.txt");
        if (!std::filesystem::exists(path)) {
            std::ofstream ofs(path);
            ofs << "USD/RUB=100.00\nEUR/RUB=110.00\n";
        }
        std::ifstream ifs(path);
        std::string all((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return QString::fromStdString(all);
    } catch (...) {
        return QStringLiteral("Курсы недоступны");
    }
}

bool BankController::isCardExpired(const QString &expiry) const {
    try {
        if (expiry.isEmpty() || expiry.length() < 5) {
            throw ValidationError("Неверный формат срока действия карты");
        }
        
        QStringList parts = expiry.split("/");
        if (parts.size() != 2) {
            throw ValidationError("Неверный формат срока действия карты");
        }
        
        bool ok;
        int month = parts[0].toInt(&ok);
        if (!ok || month < 1 || month > 12) {
            throw ValidationError("Неверный месяц в сроке действия карты");
        }
        
        int year = parts[1].toInt(&ok);
        if (!ok || year < 0 || year > 99) {
            throw ValidationError("Неверный год в сроке действия карты");
        }
        
        QDate currentDate = QDate::currentDate();
        int currentYear = currentDate.year() % 100;
        int currentMonth = currentDate.month();
        
        // Полный год (2000 + год из двух цифр)
        int fullYear = 2000 + year;
        int currentFullYear = currentDate.year();
        
        // Карта просрочена, если год меньше текущего
        if (fullYear < currentFullYear) {
            throw CardExpiredError("Карта просрочена: срок действия истек");
        }
        
        // Карта просрочена, если год равен текущему, но месяц уже прошел
        if (fullYear == currentFullYear && month < currentMonth) {
            throw CardExpiredError("Карта просрочена: срок действия истек");
        }
        
        return false; // Карта не просрочена
    } catch (const CardExpiredError &) {
        // Исключение о просрочке - возвращаем true
        return true;
    } catch (const ValidationError &) {
        // Ошибка валидации - считаем, что карта не просрочена (неверный формат)
        return false;
    } catch (...) {
        // Другие ошибки - считаем, что карта не просрочена
        return false;
    }
}

bool BankController::adjustRecipientBalance(const std::string &destination, long long deltaCents, std::string *ownerUsername) {
    bool updated = false;
    for (const auto &name : UserStorage::listUsernames()) {
        try {
            RegularUser user = UserStorage::loadUser(name);
            bool changed = false;
            auto accountIt = std::find_if(user.accounts.begin(), user.accounts.end(), [&](const Account &a){
                return a.accountNumber == destination;
            });
            if (accountIt != user.accounts.end()) {
                long long newBalance = accountIt->balanceCents + deltaCents;
                if (newBalance < 0) newBalance = 0;
                accountIt->balanceCents = newBalance;
                changed = true;
            } else {
                auto cardIt = std::find_if(user.cards.begin(), user.cards.end(), [&](const Card &c){
                    return c.cardNumber == destination;
                });
                if (cardIt != user.cards.end()) {
                    auto linked = std::find_if(user.accounts.begin(), user.accounts.end(), [&](const Account &a){
                        return a.accountNumber == cardIt->linkedAccount;
                    });
                    if (linked != user.accounts.end()) {
                        long long newBalance = linked->balanceCents + deltaCents;
                        if (newBalance < 0) newBalance = 0;
                        linked->balanceCents = newBalance;
                        changed = true;
                    }
                }
            }

            if (changed) {
                UserStorage::saveUser(user);
                if (ownerUsername) *ownerUsername = user.usernameValue;
                updated = true;
                break;
            }
        } catch (...) {
        }
    }
    return updated;
}

QVariantList BankController::listAllTransfers(const QString &query) const {
    QVariantList out;
    if (!isAdminLogin) return out;
    std::string q = query.trimmed().toStdString();
    auto users = UserStorage::loadAll();
    for (const auto &u : users) {
        for (const auto &t : u.history) {
            QVariantMap m;
            m["user"] = QString::fromStdString(u.usernameValue);
            m["id"] = QString::fromStdString(t.id);
            m["fromAccount"] = QString::fromStdString(t.fromAccount);
            m["toCard"] = QString::fromStdString(t.toCard);
            m["cents"] = static_cast<qlonglong>(t.cents);
            m["timestamp"] = static_cast<qlonglong>(t.timestamp);
            m["note"] = QString::fromStdString(t.note);
            m["status"] = QString::fromStdString(t.status);
            m["cancelReason"] = QString::fromStdString(t.cancelReason);
            if (q.empty()) { out.push_back(m); continue; }
            auto contains = [&](const std::string &s){ return s.find(q) != std::string::npos; };
            if (contains(u.usernameValue) || contains(t.id) || contains(t.fromAccount) || contains(t.toCard) || contains(t.note) || contains(t.status) || contains(t.cancelReason)) {
                out.push_back(m);
            }
        }
    }
    return out;
}


