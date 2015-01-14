#include "themeobject.hpp"

auto reg_theme_object() -> void
{
    qmlRegisterType<OsdThemeObject>("bomi", 1, 0, "OsdTheme");
    qmlRegisterType<PlaylistThemeObject>("bomi", 1, 0, "PlaylistTheme");
    qmlRegisterType<ThemeObject>("bomi", 1, 0, "Theme");
    qmlRegisterType<OsdStyleObject>("bomi", 1, 0, "OsdStyleTheme");
    qmlRegisterType<TimelineThemeObject>("bomi", 1, 0, "TimelineTheme");
    qmlRegisterType<MessageThemeObject>("bomi", 1, 0, "MessageTheme");
}
