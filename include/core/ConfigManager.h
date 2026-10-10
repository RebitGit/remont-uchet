#pragma once
#include <string>

namespace remont {

class ConfigManager {
public:
    static ConfigManager& instance();

    std::string dbPath() const;
    std::string fontPath() const;
    std::string schemaPath() const;
    int lowStockThreshold() const { return lowStockThreshold_; }

    bool autoBackupEnabled() const;
    void setAutoBackupEnabled(bool enabled);
    std::string backupDir() const;
    std::string backupSettingsPath() const;

private:
    ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::string projectRoot_;
    int lowStockThreshold_ = 3;
    bool autoBackupEnabled_ = true;
};

}