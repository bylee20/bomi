#include "settingsobject.hpp"

void reg_settings_object() {
    qmlRegisterSingletonType<SettingsObject>("CMPlayer", 1, 0, "Settings", _QmlSingleton<SettingsObject>);
}

void SettingsObject::open(const QString &name) {
if (m_name != name) {
    close();
    m_set.beginGroup("skin");
    m_set.beginGroup(name);
    m_open = true;
    m_name = name;
}
}
