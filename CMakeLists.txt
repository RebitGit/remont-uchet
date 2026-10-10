cmake_minimum_required(VERSION 3.21)
project(Remont_Uchet VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(unofficial-libharu CONFIG REQUIRED)
find_package(wxWidgets CONFIG REQUIRED)

add_library(remont_core STATIC
    src/utils/TableExport.cpp
    src/core/DatabaseManager.cpp
    src/core/ConfigManager.cpp
    src/core/Logger.cpp
    src/core/ThemeManager.cpp
    src/models/User.cpp
    src/models/Client.cpp
    src/models/Device.cpp
    src/models/Order.cpp
    src/models/Part.cpp
    src/models/PartUsage.cpp
    src/services/AuthService.cpp
    src/services/OrderService.cpp
    src/services/WarehouseService.cpp
    src/repositories/ClientRepository.cpp
    src/repositories/DeviceRepository.cpp
    src/repositories/OrderRepository.cpp
    src/repositories/PartRepository.cpp
    src/pdf/PDFGenerator.cpp
)
target_include_directories(remont_core PUBLIC include)
target_link_libraries(remont_core PUBLIC
    unofficial::sqlite3::sqlite3
    unofficial::libharu::hpdf
    wx::core wx::base
)

add_executable(Remont_Uchet
    src/main.cpp
    src/ui/LoginDialog.cpp
    src/ui/MainWindow.cpp
    src/ui/DashboardWidget.cpp
    src/ui/WarehouseWidget.cpp
    src/ui/NewOrderWidget.cpp
    src/ui/OrderViewWidget.cpp
    src/ui/AddPartDialog.cpp
    src/ui/PartFormDialog.cpp
    src/ui/ClickableListCtrl.cpp
    src/ui/SettingsDialog.cpp
    src/ui/AdminWidget.cpp
    src/ui/ReportsWidget.cpp
    src/ui/RoundedLogo.cpp
    src/ui/RoundedPanel.cpp
    app.rc
)

target_link_libraries(Remont_Uchet PRIVATE remont_core wx::core wx::base)

add_custom_command(TARGET Remont_Uchet POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:Remont_Uchet>/resources
)

enable_testing()
add_subdirectory(tests)