#include "themeobject.hpp"

auto reg_theme_object() -> void
{
    qmlRegisterType<OsdThemeObject>("CMPlayer", 1, 0, "OsdTheme");
    qmlRegisterType<PlaylistThemeObject>("CMPlayer", 1, 0, "PlaylistTheme");
    qmlRegisterType<ThemeObject>("CMPlayer", 1, 0, "Theme");
}
