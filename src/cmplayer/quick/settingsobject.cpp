#include "settingsobject.hpp"

auto reg_settings_object() -> void {
    qmlRegisterSingletonType<SettingsObject>(
        "CMPlayer", 1, 0, "Settings", _QmlSingleton<SettingsObject>
    );
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
