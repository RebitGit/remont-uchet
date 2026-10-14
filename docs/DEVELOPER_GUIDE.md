# Руководство разработчика

## Требования к окружению

- Windows 10 или 11 (64-бит)
- MSYS2 с MINGW64
- MinGW-w64 GCC 16.2.0
- CMake 3.21+
- Ninja
- vcpkg
- Git
- VSCodium с расширениями C/C++, CMake Tools

## Установка окружения

### 1. MSYS2

Скачать с https://www.msys2.org, установить в D:\mingw64.

Обновить пакеты:

pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-binutils

Проверить версию:

g++ --version

Должно быть: g++ (Rev5, Built by MSYS2 project) 16.2.0 или новее.

### 2. vcpkg

Установить в C:\vcpkg:

git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

Установить зависимости:

C:\vcpkg\vcpkg.exe install wxwidgets libharu sqlite3 --triplet x64-mingw-dynamic

### 3. Проект

git clone https://github.com/RebitGit/remont-uchet.git D:\Projects\Remont_Uchet
cd D:\Projects\Remont_Uchet

## Сборка

### Через скрипт

build.bat

### Вручную

mkdir build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic -DVCPKG_HOST_TRIPLET=x64-mingw-dynamic -DCMAKE_C_COMPILER=D:/mingw64/bin/gcc.exe -DCMAKE_CXX_COMPILER=D:/mingw64/bin/g++.exe
cmake --build . -j8

## Запуск

run.bat

Или: build\Remont_Uchet.exe

Первый запуск создаёт БД data/remont_uchet.db и пользователя admin/admin.

## Тесты

cd build
ctest --output-on-failure

Должно быть 100% tests passed out of 3.

## Структура проекта

- CMakeLists.txt — главный файл сборки
- README.md, BACKLOG.md, CHANGELOG.md, SCRUM.md
- build.bat, run.bat, package.bat
- app.rc — манифест wx
- include/ — заголовочные файлы
- src/ — реализации
- resources/ — иконки, шрифты
- sql/ — схема БД
- tests/ — модульные тесты
- docs/ — документация

## Ключевые точки в коде

| Файл | Назначение |
|---|---|
| src/main.cpp | точка входа |
| src/ui/MainWindow.cpp | главное окно и навигация |
| src/ui/OrderViewWidget.cpp | экран заказа |
| src/services/OrderService.cpp | бизнес-логика заказов |
| src/services/WarehouseService.cpp | бизнес-логика склада |
| src/repositories/OrderRepository.cpp | работа с таблицей orders |
| src/pdf/PDFGenerator.cpp | генерация PDF-актов |
| sql/schema.sql | схема БД |

## Добавление новой функции

1. Создать feature-ветку: git checkout -b feature/название
2. Изменения по слоям снизу вверх: модель, репозиторий, сервис, UI.
3. Обновить CMakeLists.txt при новых .cpp.
4. Собрать и проверить.
5. Написать тесты.
6. git add . && git commit -m "feat: описание"
7. git push -u origin feature/название
8. Открыть Pull Request на GitHub.
9. После проверки — merge в main.

## Соглашения по коду

- Стиль — .clang-format (Google, 4 пробела).
- Проверки — .clang-tidy.
- Комментарии — Doxygen в публичных методах.
- Русские строки — через utf8::U().
- SQL — только параметризованный.

## Частые проблемы

### ninja: failed recompaction: Permission denied
VSCodium держит папку build/. Закрыть редактор, удалить build/, пересобрать.

### STATUS_ENTRYPOINT_NOT_FOUND при запуске
Несовпадение версий libstdc++ и пакетов vcpkg. Пересобрать vcpkg через --no-binarycaching.

### Краткозябры
Файлы не в UTF-8. Проверить кодировку.

### No tests were found в ctest
Проверить tests/CMakeLists.txt — должны быть add_test.
