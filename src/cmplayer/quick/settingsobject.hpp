#ifndef SETTINGSOBJECT_HPP
#define SETTINGSOBJECT_HPP

#include "stdafx.hpp"

class SettingsObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name)
public:
    ~SettingsObject() { close(); }
    QString name() const { return m_name; }
    Q_INVOKABLE void open(const QString &name);
    Q_INVOKABLE void close() { if (m_open) {m_set.endGroup(); m_set.endGroup(); m_open = false; m_name.clear();} }
    Q_INVOKABLE void set(const QString &key, const QVariant &var) { if (m_open) m_set.setValue(key, var); }
    Q_INVOKABLE bool getBool(const QString &key, bool def) const { return get(key, def).toBool(); }
    Q_INVOKABLE int getInt(const QString &key, int def) const { return get(key, def).toInt(); }
    Q_INVOKABLE qreal getReal(const QString &key, qreal def) const { return get(key, def).toReal(); }
    Q_INVOKABLE QString getString(const QString &key, const QString &def) const { return get(key, def).toString(); }
    QVariant get(const QString &key, const QVariant &def) const { return m_open ? m_set.value(key, def) : QVariant(); }
private:
    QString m_name; bool m_open = false; QSettings m_set;
};

#endif // SETTINGSOBJECT_HPP
