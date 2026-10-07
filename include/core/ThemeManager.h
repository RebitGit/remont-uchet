#pragma once
#include <wx/wx.h>
#include <string>

namespace remont {

enum class Theme { Light, Dark, System };
enum class TextSize { Small, Normal, Large };

class ThemeManager {
public:
    static ThemeManager& instance();

    void load();
    void save();

    Theme theme() const { return theme_; }
    TextSize textSize() const { return textSize_; }

    void setTheme(Theme t);
    void setTextSize(TextSize s);

    bool isDark() const;

    wxColour background() const;
    wxColour surface() const;
    wxColour text() const;
    wxColour muted() const;
    wxColour border() const;
    wxColour sidebar() const;
    wxColour sidebarHover() const;
    wxColour primary() const;
    wxColour success() const;
    wxColour warning() const;
    wxColour danger() const;

    int fontSizeSmall() const;
    int fontSizeNormal() const;
    int fontSizeLarge() const;
    int fontSizeTitle() const;
    int fontSizeHeader() const;

private:
    ThemeManager() = default;

    Theme theme_ = Theme::Light;
    TextSize textSize_ = TextSize::Normal;

    std::string settingsPath() const;
};

}