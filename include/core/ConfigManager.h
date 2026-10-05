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

private:
    ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::string projectRoot_;
    int lowStockThreshold_ = 3;
};

}
