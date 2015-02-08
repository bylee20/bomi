#include "themeobject.hpp"

auto reg_theme_object() -> void
{
    qmlRegisterType<ThemeObject>();
    qmlRegisterType<PlaylistThemeObject>();
    qmlRegisterType<HistoryThemeObject>();
    qmlRegisterType<OsdThemeObject>();
    qmlRegisterType<OsdStyleObject>("bomi", 1, 0, "OsdStyleTheme");
    qmlRegisterType<TimelineThemeObject>("bomi", 1, 0, "TimelineTheme");
    qmlRegisterType<MessageThemeObject>();
}

auto ThemeObject::monospace() const -> QString
{
    return u"monospace"_q;
}
