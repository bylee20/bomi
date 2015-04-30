#include "settingsobject.hpp"

SettingsObject::SettingsObject()
    : m_set(_WritablePath(Location::Config) % "/skinsettings.ini"_a, QSettings::IniFormat)
{

}

auto SettingsObject::open(const QString &name) -> void
{
    if (m_name != name) {
        close();
        m_set.beginGroup(u"skin"_q);
        m_set.beginGroup(name);
        m_open = true;
        m_name = name;
    }
}
