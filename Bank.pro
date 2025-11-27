QT += core qml quick

# Убираем widgets, если не используем QWidget
# QT += widgets  # ← убираем, если не нужен

CONFIG += c++17

TARGET = BankApp
TEMPLATE = app

# Исходные файлы
SOURCES += \
    src/main.cpp \
    src/controller/BankController.cpp \
    src/controller/AuthController.cpp \
    src/controller/AccountController.cpp \
    src/controller/TransactionController.cpp \
    src/controller/AdminController.cpp

# Заголовочные файлы
HEADERS += \
    src/controller/BankController.h \
    src/controller/AuthController.h \
    src/controller/AccountController.h \
    src/controller/TransactionController.h \
    src/controller/AdminController.h \
    src/models/Account.h \
    src/models/Card.h \
    src/models/FavoritePayment.h \
    src/models/Payment.h \
    src/models/Transaction.h \
    src/models/User.h \
    src/storage/UserStorage.h \
    src/utils/Exceptions.h \
    src/utils/Utils.h

# Пути для поиска заголовков
INCLUDEPATH += src

# Ресурсы (QML и, возможно, иконки и т.п.)
RESOURCES += qml.qrc

# Для отладки
CONFIG += debug

# C++17 явно (в Qt6 обычно не нужно, но не помешает)
QMAKE_CXXFLAGS += -std=c++17
