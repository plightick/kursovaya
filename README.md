# Банковская система — курсовая работа

Курсовой проект на C++/Qt, реализующий упрощённую банковскую систему. Исходный код расположен в папке `src/`.

## Возможности
- Авторизация и управление пользователями
- Управление счетами и картами
- Проведение и история транзакций
- Избранные платежи
- Уведомления
- Роли/администрирование

## Архитектура
- **Контроллеры (`src/controller/`)**: бизнес-логика и слой взаимодействия с UI
- **Модели (`src/models/`)**: доменные сущности (пользователь, счёт, карта, транзакция и т.д.)
- **Хранилище (`src/storage/`)**: абстракции хранения/доступа к данным
- **Утилиты (`src/utils/`)**: исключения, вспомогательные функции
- **Входная точка (`src/main.cpp`)**

Диаграммы классов (PlantUML) находятся в `gen/*.puml`.

## Структура проекта
```
kursovaya/
├─ src/
│  ├─ controller/
│  │  ├─ AccountController.{h,cpp}
│  │  ├─ AdminController.{h,cpp}
│  │  ├─ AuthController.{h,cpp}
│  │  ├─ BankController.{h,cpp}
│  │  ├─ BaseController.{h,cpp}
│  │  ├─ NotificationController.{h,cpp}
│  │  └─ TransactionController.{h,cpp}
│  ├─ models/
│  │  ├─ Account.h
│  │  ├─ Card.h
│  │  ├─ FavoritePayment.h
│  │  ├─ Payment.h
│  │  ├─ Transaction.h
│  │  └─ User.h
│  ├─ storage/
│  │  └─ UserStorage.h
│  ├─ utils/
│  │  ├─ Exceptions.h
│  │  └─ Utils.h
│  └─ main.cpp
├─ gen/
│  ├─ BankClassDiagram_*.puml
│  ├─ test.py          # экспорт исходников в DOCX
│  └─ 3.docx           # шаблон документа
├─ docs/
│  └─ Описание_функций*.docx
└─ Bank.pro            # Qt/qmake проект
```

## Технологии
- C++17
- Qt 6 (проверено на Qt 6.10)
- QML/Qt Quick (если используется в UI)
- PlantUML (диаграммы)

## Сборка и запуск
### Вариант 1. Qt Creator (рекомендуется)
1. Откройте файл `Bank.pro`
2. Выберите профиль сборки (Debug/Release)
3. Нажмите Build/Run

### Вариант 2. qmake (CLI)
```bash
# в каталоге kursovaya/
qmake6 Bank.pro
make -j
# запуск бинарника через Qt Creator или из папки сборки
```

> Примечание: точное имя исполняемого файла зависит от конфигурации проекта/окружения.

## Диаграммы UML (PlantUML)
- Исходники диаграмм: `gen/BankClassDiagram_*.puml`
- Генерация PNG:
```bash
# пример (понадобится установленный plantuml)
plantuml gen/BankClassDiagram_full.puml
```

## Экспорт исходников проекта в DOCX
В папке `gen/` есть скрипт `test.py`, который собирает исходники из `src/` в файл `res.docx` (шрифт Courier New). Скрипт использует шаблон `3.docx` из той же папки.

```bash
# запускать из папки gen/
python3 test.py ../src
# результат: gen/res.docx
```

## Стиль и качество
- Именование и разбиение по слоям: Controller / Model / Utils / Storage
- Исключения и обработка ошибок — через `src/utils/Exceptions.h`
- При необходимости добавляйте тесты/логи для ключевой логики

## Документация
- Файлы `.docx` в `docs/` (основной текст — Times New Roman, код — Courier New)
- Дополнительно: UML-диаграммы (PlantUML) в `gen/`

## Планы развития
- Подключение реального хранилища (БД)
- Журналирование действий
- Тестовое покрытие ключевых сценариев

## Автор
Курсовая работа «Банковская система».
