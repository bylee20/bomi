#ifndef SETTINGSOBJECT_HPP
#define SETTINGSOBJECT_HPP

#include <QQuickItem>
#include <QSettings>

class SettingsObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name)
public:
    SettingsObject();
    ~SettingsObject() { close(); }
    auto name() const -> QString { return m_name; }
    auto get(const QString &key, const QVariant &def) const -> QVariant;
    Q_INVOKABLE void open(const QString &name);
    Q_INVOKABLE void close();
    Q_INVOKABLE void set(const QString &key, const QVariant &var);
    Q_INVOKABLE bool getBool(const QString &key, bool def) const;
    Q_INVOKABLE int getInt(const QString &key, int def) const;
    Q_INVOKABLE qreal getReal(const QString &key, qreal def) const;
    Q_INVOKABLE QString getString(const QString &key, const QString &def) const;
private:
    QString m_name; bool m_open = false; QSettings m_set;
};

inline auto SettingsObject::get(const QString &key,
                                const QVariant &def) const -> QVariant
{ return m_open ? m_set.value(key, def) : QVariant(); }

inline auto SettingsObject::close() -> void
{
    if (m_open) {
        m_set.endGroup();
        m_set.endGroup();
        m_open = false;
        m_name.clear();
    }
}

inline auto SettingsObject::set(const QString &key, const QVariant &var) -> void
{ if (m_open) m_set.setValue(key, var); }

inline auto SettingsObject::getBool(const QString &key, bool def) const -> bool
{ return get(key, def).toBool(); }

inline auto SettingsObject::getInt(const QString &key, int def) const -> int
{ return get(key, def).toInt(); }

inline auto SettingsObject::getReal(const QString &key,
                                    qreal def) const -> qreal
{ return get(key, def).toReal(); }

inline auto SettingsObject::getString(const QString &key,
                                      const QString &def) const -> QString
{ return get(key, def).toString(); }


#endif // SETTINGSOBJECT_HPP
