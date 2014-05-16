#include "themeobject.hpp"
#include "osdtheme.hpp"

auto reg_theme_object() -> void
{
    qmlRegisterType<OsdThemeObject>("CMPlayer", 1, 0, "OsdTheme");
    qmlRegisterType<ThemeObject>("CMPlayer", 1, 0, "Theme");
}

ThemeObject::ThemeObject(QObject *parent)
    : QObject(parent)
{
    m_osd = new OsdThemeObject(this);
}

auto ThemeObject::setOsd(const OsdTheme &osd) -> void
{
    m_osd->set(osd);
    emit osdChanged();
}
