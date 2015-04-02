#ifndef THEMEOBJECT_HPP
#define THEMEOBJECT_HPP

#include "playlistthemeobject.hpp"
#include "osdthemeobject.hpp"

class ThemeObject : public QObject {
    Q_OBJECT
#define P_(type, name) \
private: \
    Q_PROPERTY(type##Object *name READ __##name NOTIFY name##Changed) \
    type##Object m_##name; \
public: \
    auto __##name() const -> type##Object* { return (type##Object*)&m_##name; } \
    auto set(const type &t) -> void { m_##name.m = t; emit name##Changed(); } \
    Q_SIGNAL void name##Changed(); \
private:
    P_(ControlsTheme, controls)
#undef P_
private:
    Q_PROPERTY(OsdThemeObject *osd READ __osd NOTIFY osdChanged)
    Q_PROPERTY(QString monospace READ monospace CONSTANT FINAL)
    Q_PROPERTY(QFont font READ font CONSTANT FINAL)
    OsdThemeObject m_osd;
public:
    Q_SIGNAL void osdChanged();
    auto __osd() const -> OsdThemeObject* { return (OsdThemeObject*)&m_osd; }
    auto set(const OsdTheme &t) -> void {
        m_osd.m.style.m = t.style;
        m_osd.m.timeline.m = t.timeline;
        m_osd.m.message.m = t.message;
        emit osdChanged();
    }
    auto font() const -> QFont;
    auto monospace() const -> QString;
};

#endif // THEMEOBJECT_HPP
