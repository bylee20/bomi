#ifndef THEMEOBJECT_HPP
#define THEMEOBJECT_HPP

#include "playlistthemeobject.hpp"
#include "osdthemeobject.hpp"

class ThemeObject : public QObject {
    Q_OBJECT
#define P_(type, name) \
private: \
    type m_##name; \
    Q_PROPERTY(type *name READ name CONSTANT FINAL) \
public: \
    auto name() -> type* { return &m_##name; } \
private:
    P_(OsdThemeObject, osd)
    P_(PlaylistThemeObject, playlist)
#undef P_
};

#endif // THEMEOBJECT_HPP
