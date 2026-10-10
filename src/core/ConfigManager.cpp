#include "core/ConfigManager.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace remont {

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

ConfigManager::ConfigManager() {
    fs::path thisFile = __FILE__;
    projectRoot_ = thisFile.parent_path().parent_path().parent_path().string();

    std::ifstream f(backupSettingsPath());
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (key == "auto_backup") {
                autoBackupEnabled_ = (val == "1" || val == "true");
            }
        }
    }
}

std::string ConfigManager::dbPath() const {
    return (fs::path(projectRoot_) / "data" / "remont_uchet.db").string();
}

std::string ConfigManager::fontPath() const {
    return (fs::path(projectRoot_) / "resources" / "fonts" / "DejaVuSans.ttf").string();
}

std::string ConfigManager::schemaPath() const {
    return (fs::path(projectRoot_) / "sql" / "schema.sql").string();
}

std::string ConfigManager::backupDir() const {
    return (fs::path(dbPath()).parent_path() / ".." / "backups").string();
}

std::string ConfigManager::backupSettingsPath() const {
    return (fs::path(dbPath()).parent_path() / "backup_settings.ini").string();
}

bool ConfigManager::autoBackupEnabled() const {
    return autoBackupEnabled_;
}

void ConfigManager::setAutoBackupEnabled(bool enabled) {
    autoBackupEnabled_ = enabled;

    std::error_code ec;
    fs::create_directories(fs::path(backupSettingsPath()).parent_path(), ec);

    std::ofstream f(backupSettingsPath());
    if (f.is_open()) {
        f << "auto_backup=" << (enabled ? "1" : "0") << "\n";
    }
}

}