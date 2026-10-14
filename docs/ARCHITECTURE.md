# Архитектура АС «Ремонт-Учёт»

## Общий стиль

Приложение построено по многослойной архитектуре (Layered Architecture) внутри единого монолитного исполняемого файла.

Три слоя:

1. UI — экраны и виджеты (wxWidgets).
2. Сервисы — бизнес-логика.
3. Репозитории — доступ к БД SQLite.

Слои не пересекаются напрямую: UI вызывает сервисы, сервисы вызывают репозитории, репозитории работают с БД.

## Слой представления (UI)

Расположен в src/ui/. Содержит:

- Экраны: LoginDialog, MainWindow, DashboardWidget, NewOrderWidget, OrderViewWidget, WarehouseWidget, ReportsWidget, AdminWidget.
- Диалоги: SettingsDialog, AddPartDialog, PartFormDialog.
- Вспомогательные виджеты: RoundedPanel, RoundedLogo, ClickableListCtrl.
- Кастомные компоненты: CircleAvatar, LogoBox, BarChartPanel, DonutChartPanel, StatsTablePanel.

Задачи UI:

- Отображение данных.
- Обработка событий пользователя.
- Вызов сервисов для выполнения операций.
- Никаких SQL-запросов — только через сервисы и репозитории.

## Слой бизнес-логики (Services)

Расположен в src/services/. Содержит:

- AuthService — аутентификация, разграничение прав.
- OrderService — создание заказа, смена статуса, печать акта, автоудаление устаревших заказов.
- WarehouseService — списание запчастей, пересчёт стоимости заказа.

Задачи сервисов:

- Реализация бизнес-правил.
- Оркестрация нескольких репозиториев в одной операции.
- Логирование действий пользователя через Logger.

## Слой доступа к данным (Repositories)

Расположен в src/repositories/. Содержит:

- ClientRepository — клиенты.
- DeviceRepository — устройства.
- OrderRepository — заказы.
- PartRepository — запчасти.

Задачи репозиториев:

- Выполнение SQL-запросов.
- Параметризация всех запросов через sqlite3_bind_* — защита от SQL-инъекций.
- Возврат структур из src/models/.

## Инфраструктура

Расположена в src/core/:

- DatabaseManager — Singleton, соединение с SQLite.
- ConfigManager — Singleton, пути и настройки автобэкапа.
- ThemeManager — Singleton, светлая и тёмная палитры.
- Logger — запись действий в operation_log.
- Types — общие типы, проверки прав canAccess*.

## Генерация PDF

Расположена в src/pdf/:

- PDFGenerator — Singleton, генерация акта приёма-передачи.

## Утилиты

Расположены в src/utils/:

- TableExport — экспорт в CSV и генерация таблиц в PDF.

## Паттерны проектирования

### MVC (Model-View-Controller)

- Model — структуры в src/models/ (User, Client, Device, Order, Part).
- View — виджеты wxWidgets.
- Controller — виджеты-экраны, обрабатывающие события пользователя.

### Repository

Все обращения к БД — только через репозитории. UI и сервисы не знают, какая СУБД используется.

### Singleton

Реализован для DatabaseManager, ConfigManager, ThemeManager, Logger, OrderService, WarehouseService, AuthService, PDFGenerator.

Гарантирует единый экземпляр и общее состояние во всех частях приложения.

## Схема данных

База данных SQLite. 7 таблиц:

| Таблица | Назначение |
|---|---|
| users | Пользователи системы |
| clients | Клиенты сервисного центра |
| devices | Устройства, принятые в ремонт |
| orders | Заказы |
| parts | Запчасти на складе |
| part_usage | Списания запчастей на заказы |
| operation_log | Журнал операций |

Полная схема — в sql/schema.sql. Нормализация — до 3NF.

## Диаграмма развёртывания

Один узел — клиентский ПК под Windows 10/11.

- Приложение Remont_Uchet.exe.
- Файл БД data/remont_uchet.db.
- Папка ресурсов resources/.
- Папка резервных копий backups/.

Отдельный сервер БД не требуется, SQLite работает как встраиваемая СУБД.

## Поток выполнения запроса

Пример: пользователь нажимает «Печать акта» в карточке заказа.

1. OrderViewWidget::onPrint — обработчик клика.
2. OrderService::printAcceptanceAct(orderId, outputPath) — вызов сервиса.
3. OrderRepository::findById(orderId) — получение заказа из БД.
4. ClientRepository::findById и DeviceRepository::findById — связанные данные.
5. PDFGenerator::generateAcceptanceAct — генерация PDF.
6. Logger::log — запись в operation_log.
7. Возврат результата в UI, показ сообщения.

## Ключевые решения

- Монолит без внешнего API. Проще для учебного проекта.
- Нативные SQL-запросы вместо ORM. Полный контроль, явная параметризация.
- wxWidgets вместо Qt. Свободная лицензия, нативная отрисовка под Windows.
- SQLite вместо PostgreSQL. Встраиваемая, не требует сервера.
