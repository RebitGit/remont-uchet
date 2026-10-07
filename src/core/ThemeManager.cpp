#include "core/ThemeManager.h"
#include "core/ConfigManager.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace remont {

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

std::string ThemeManager::settingsPath() const {
    fs::path root = fs::path(ConfigManager::instance().dbPath()).parent_path();
    return (root / "settings.ini").string();
}

void ThemeManager::load() {
    theme_ = Theme::Light;
    textSize_ = TextSize::Normal;

    std::ifstream f(settingsPath());
    if (!f.is_open()) {
        std::cerr << "[THEME] settings.ini not found, using defaults\n";
        return;
    }

    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if (key == "theme") {
            if (val == "dark") theme_ = Theme::Dark;
            else if (val == "system") theme_ = Theme::System;
            else theme_ = Theme::Light;
        } else if (key == "text_size") {
            if (val == "small") textSize_ = TextSize::Small;
            else if (val == "large") textSize_ = TextSize::Large;
            else textSize_ = TextSize::Normal;
        }
    }

    std::cerr << "[THEME] loaded: theme="
              << (theme_ == Theme::Dark ? "dark" :
                  theme_ == Theme::System ? "system" : "light")
              << ", size="
              << (textSize_ == TextSize::Small ? "small" :
                  textSize_ == TextSize::Large ? "large" : "normal")
              << "\n";
}

void ThemeManager::save() {
    std::ofstream f(settingsPath());
    if (!f.is_open()) {
        std::cerr << "[THEME] cannot save settings to " << settingsPath() << "\n";
        return;
    }

    std::string t = "light";
    if (theme_ == Theme::Dark) t = "dark";
    else if (theme_ == Theme::System) t = "system";

    std::string s = "normal";
    if (textSize_ == TextSize::Small) s = "small";
    else if (textSize_ == TextSize::Large) s = "large";

    f << "theme=" << t << "\n";
    f << "text_size=" << s << "\n";
    f.close();

    std::cerr << "[THEME] saved: theme=" << t << ", size=" << s << "\n";
}

void ThemeManager::setTheme(Theme t) {
    theme_ = t;
    save();
}

void ThemeManager::setTextSize(TextSize s) {
    textSize_ = s;
    save();
}

bool ThemeManager::isDark() const {
    return theme_ == Theme::Dark;
}

wxColour ThemeManager::background() const {
    if (isDark()) return wxColour(0x11, 0x18, 0x27);
    return wxColour(0xF9, 0xFA, 0xFB);
}

wxColour ThemeManager::surface() const {
    if (isDark()) return wxColour(0x1E, 0x29, 0x3B);
    return wxColour(0xFF, 0xFF, 0xFF);
}

wxColour ThemeManager::text() const {
    if (isDark()) return wxColour(0xF1, 0xF5, 0xF9);
    return wxColour(0x11, 0x18, 0x27);
}

wxColour ThemeManager::muted() const {
    if (isDark()) return wxColour(0x94, 0xA3, 0xB8);
    return wxColour(0x6B, 0x72, 0x80);
}

wxColour ThemeManager::border() const {
    if (isDark()) return wxColour(0x33, 0x41, 0x55);
    return wxColour(0xE5, 0xE7, 0xEB);
}

wxColour ThemeManager::sidebar() const {
    if (isDark()) return wxColour(0x0F, 0x17, 0x2A);
    return wxColour(0x1E, 0x29, 0x3B);
}

wxColour ThemeManager::sidebarHover() const {
    if (isDark()) return wxColour(0x1E, 0x29, 0x3B);
    return wxColour(0x33, 0x41, 0x55);
}

wxColour ThemeManager::primary() const {
    return wxColour(0x25, 0x63, 0xEB);
}

wxColour ThemeManager::success() const {
    return wxColour(0x10, 0xB9, 0x81);
}

wxColour ThemeManager::warning() const {
    return wxColour(0xF5, 0x9E, 0x0B);
}

wxColour ThemeManager::danger() const {
    return wxColour(0xEF, 0x44, 0x44);
}

int ThemeManager::fontSizeSmall() const {
    switch (textSize_) {
        case TextSize::Small: return 10;
        case TextSize::Large: return 14;
        default: return 12;
    }
}

int ThemeManager::fontSizeNormal() const {
    switch (textSize_) {
        case TextSize::Small: return 11;
        case TextSize::Large: return 15;
        default: return 13;
    }
}

int ThemeManager::fontSizeLarge() const {
    switch (textSize_) {
        case TextSize::Small: return 13;
        case TextSize::Large: return 18;
        default: return 15;
    }
}

int ThemeManager::fontSizeTitle() const {
    switch (textSize_) {
        case TextSize::Small: return 18;
        case TextSize::Large: return 26;
        default: return 22;
    }
}

int ThemeManager::fontSizeHeader() const {
    switch (textSize_) {
        case TextSize::Small: return 14;
        case TextSize::Large: return 20;
        default: return 16;
    }
}

}