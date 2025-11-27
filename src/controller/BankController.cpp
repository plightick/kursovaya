#include "BankController.h"
#include "storage/UserStorage.h"
#include "utils/Exceptions.h"
#include "utils/Utils.h"
#include <QDate>
#include <filesystem>
#include <fstream>

using namespace utils;

BankController::BankController(QObject *parent) : QObject(parent) {
    connect(&m_authController, &AuthController::authenticatedChanged, this, &BankController::onAuthenticatedChanged);
    connect(&m_authController, &AuthController::errorOccured, this, &BankController::errorOccured);
    connect(&m_authController, &AuthController::infoMessage, this, &BankController::infoMessage);
}

void BankController::login(const QString &username, const QString &password) {
    m_authController.login(username, password);
}

void BankController::logout() {
    m_authController.logout();
}

void BankController::registerUser(const QString &username, const QString &password) {
    m_authController.registerUser(username, password);
}

bool BankController::isAuthenticated() const {
    return m_authController.isAuthenticated();
}

bool BankController::isAdmin() const {
    return m_authController.isAdmin();
}

QString BankController::username() const {
    return m_authController.username();
}

void BankController::onAuthenticatedChanged() {
    delete m_accountController;
    m_accountController = nullptr;
    delete m_transactionController;
    m_transactionController = nullptr;
    delete m_adminController;
    m_adminController = nullptr;

    if (m_authController.isAuthenticated()) {
        if (m_authController.isAdmin()) {
            m_adminController = new AdminController(this);
            connect(m_adminController, &AdminController::errorOccured, this, &BankController::errorOccured);
            connect(m_adminController, &AdminController::infoMessage, this, &BankController::infoMessage);
        } else {
            m_accountController = new AccountController(m_authController.getCurrentUser(), this);
            m_transactionController = new TransactionController(m_authController.getCurrentUser(), this);
            connect(m_accountController, &AccountController::errorOccured, this, &BankController::errorOccured);
            connect(m_accountController, &AccountController::infoMessage, this, &BankController::infoMessage);
            connect(m_transactionController, &TransactionController::errorOccured, this, &BankController::errorOccured);
            connect(m_transactionController, &TransactionController::infoMessage, this, &BankController::infoMessage);
        }
    }
    emit authenticatedChanged();
}

QVariantList BankController::listAccounts() const {
    if (!m_accountController) return {};
    return m_accountController->listAccounts();
}

QVariantList BankController::listCards() const {
    if (!m_accountController) return {};
    return m_accountController->listCards();
}

void BankController::addAccount(const QString &currency) {
    if (!m_accountController) return;
    m_accountController->addAccount(currency);
}

void BankController::addCard(const QString &holderName, const QString &expiry, const QString &linkedAccount) {
    if (!m_accountController) return;
    m_accountController->addCard(holderName, expiry, linkedAccount);
}

QVariantList BankController::listHistory() const {
    if (!m_transactionController) return {};
    return m_transactionController->listHistory();
}

QVariantList BankController::listFavorites() const {
    if (!m_transactionController) return {};
    return m_transactionController->listFavorites();
}

void BankController::addFavorite(const QString &name, const QString &toCard, const QString &note) {
    if (!m_transactionController) return;
    m_transactionController->addFavorite(name, toCard, note);
}

void BankController::transfer(const QString &fromAccount, const QString &toCard, qlonglong cents, const QString &note, const QString &category) {
    if (!m_transactionController) return;
    m_transactionController->transfer(fromAccount, toCard, cents, note, category);
}

void BankController::payFavorite(const QString &favName, const QString &fromAccount, qlonglong cents, const QString &category) {
    if (!m_transactionController) return;
    m_transactionController->payFavorite(favName, fromAccount, cents, category);
}

QVariantMap BankController::getExpenseStats() const {
    if (!m_transactionController) return {};
    return m_transactionController->getExpenseStats();
}

void BankController::depositToAccount(const QString &accountNumber, qlonglong cents, const QString &externalAccount) {
    if (!m_transactionController) return;
    m_transactionController->depositToAccount(accountNumber, cents, externalAccount);
}

QVariantMap BankController::receiptFor(const QString &transactionId) const {
    if (!m_transactionController) return {};
    return m_transactionController->receiptFor(transactionId);
}

QString BankController::downloadReceipt(const QString &transactionId) {
    if (!m_transactionController) return {};
    return m_transactionController->downloadReceipt(transactionId);
}

QString BankController::saveReceiptToFile(const QString &transactionId, const QString &filePath) {
    if (!m_transactionController) return {};
    return m_transactionController->saveReceiptToFile(transactionId, filePath);
}

void BankController::setAccountBalance(const QString &accountNumber, qlonglong cents) {
    emit errorOccured(QStringLiteral("Нельзя напрямую установить баланс для счета %1. Используйте пополнение на %2 коп.")
                          .arg(accountNumber)
                          .arg(cents));
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
    } catch (const std::filesystem::filesystem_error &) {
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
    }
}

bool BankController::adjustRecipientBalance(std::string_view destination, long long deltaCents, std::string *ownerUsername) const {
    const std::string destStr(destination);
    for (const auto &name : UserStorage::listUsernames()) {
        try {
            RegularUser user = UserStorage::loadUser(name);
            Account *acc = findAccount(user, destStr);
            if (!acc) acc = findLinkedAccount(user, destStr);
            if (!acc) continue;
            long long newBalance = acc->balanceCents + deltaCents;
            acc->balanceCents = newBalance < 0 ? 0 : newBalance;
            UserStorage::saveUser(user);
            if (ownerUsername) *ownerUsername = user.usernameValue;
            return true;
        } catch (const BankingError &) {
            continue;
        }
    }
    return false;
}

QVariantList BankController::listAllTransfers(const QString &query) const {
    QVariantList out;
    if (!isAdminLogin) return out;
    auto q = query.trimmed();
    auto users = UserStorage::loadAll();
    for (const auto &u : users) {
        for (const auto &t : u.history) {
            QVariantMap m;
            m["user"] = QString::fromStdString(u.usernameValue);
            m["id"] = QString::fromStdString(t.id);
            m["fromAccount"] = QString::fromStdString(t.fromAccount);
            m["toCard"] = QString::fromStdString(t.toCard);
            m["cents"] = t.cents;
            m["timestamp"] = t.timestamp;
            m["note"] = QString::fromStdString(t.note);
            m["status"] = QString::fromStdString(t.status);
            m["cancelReason"] = QString::fromStdString(t.cancelReason);
            if (q.isEmpty()) { out.push_back(m); continue; }
            auto qsu = QString::fromStdString(u.usernameValue);
            auto qid = QString::fromStdString(t.id);
            auto qfrom = QString::fromStdString(t.fromAccount);
            auto qto = QString::fromStdString(t.toCard);
            auto qnote = QString::fromStdString(t.note);
            auto qstatus = QString::fromStdString(t.status);
            auto qreason = QString::fromStdString(t.cancelReason);
            if (qsu.contains(q) || qid.contains(q) || qfrom.contains(q) || qto.contains(q) || qnote.contains(q) || qstatus.contains(q) || qreason.contains(q)) {
                out.push_back(m);
            }
        }
    }
    return out;
}


