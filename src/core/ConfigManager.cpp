#include "core/ConfigManager.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace remont {

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

ConfigManager::ConfigManager() {
    fs::path thisFile = __FILE__;
    projectRoot_ = thisFile.parent_path().parent_path().parent_path().string();
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

}