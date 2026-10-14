# API приложения

Внешнего REST API нет. Внутренние интерфейсы между слоями описаны ниже.

## DatabaseManager

Singleton. Соединение с SQLite.

- instance() — получить экземпляр
- open(path) — открыть БД
- close() — закрыть
- handle() — sqlite3* для прямых запросов
- execFile(path) — выполнить SQL-скрипт из файла

## ConfigManager

Singleton. Пути и настройки.

- dbPath() — путь к БД
- fontPath() — путь к шрифту DejaVuSans
- schemaPath() — путь к schema.sql
- backupDir() — папка бэкапов
- autoBackupEnabled() — включён ли автобэкап
- setAutoBackupEnabled(enabled)

## Logger

Singleton. Запись в operation_log.

- log(userId, action, entityType, entityId)

## AuthService

- authenticate(login, password, outUser) — проверить логин/пароль
- seedAdminIfMissing() — создать admin/admin, если нет

## OrderService

- createOrder(clientFullName, clientPhone, clientEmail, deviceType, deviceModel, deviceSerial, description, totalCost, userId, outOrder, masterId) — создать заказ
- changeStatus(orderId, newStatus) — сменить статус
- printAcceptanceAct(orderId, outputPath) — сохранить PDF
- cleanupOldIssuedOrders(days) — удалить старые выданные заказы

## WarehouseService

- writeOffPart(partId, quantity, orderId, userId) — списать запчасть
- getLowStockParts() — список с низким остатком

## Репозитории

### ClientRepository

- add(client), update(client), remove(id)
- findById(id), findByPhone(phone), getAll()

### DeviceRepository

- add(device), update(device), findById(id)

### OrderRepository

- add(order), update(order), updateStatus(orderId, status), remove(id)
- findById(id), getAll(), getByStatus(status), getByClient(clientId)
- generateNumber() — сгенерировать номер ORD-2026-XXXX

### PartRepository

- add(part), update(part), remove(id), writeOff(partId, quantity)
- findById(id), findByArticle(article), getAll(), getLowStock()

## PDFGenerator

- generateAcceptanceAct(order, client, device, outputPath, fontPath) — создать PDF-акт

## TableExport

- exportCsv(data, path) — сохранить CSV
- printPdf(data, path, fontPath, line1, line2) — сохранить PDF-таблицу
- timestamp() — текущая метка времени

## ThemeManager

- setTheme(theme), theme()
- primary(), background(), surface(), text(), muted(), border(), success(), warning(), danger(), sidebar(), sidebarHover()
- fontSizeSmall(), fontSizeNormal(), fontSizeHeader(), fontSizeTitle()

## Проверки прав

В core/Types.h:

- canAccessOrders(role), canWriteOrders(role)
- canAccessWarehouse(role), canWriteWarehouse(role)
- canAccessReports(role), canAccessAdmin(role)

## Обработка ошибок

Сервисы возвращают bool. UI показывает wxMessageBox при ошибке. Репозитории возвращают пустые структуры (id == 0).
