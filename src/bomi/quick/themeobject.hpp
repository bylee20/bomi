#ifndef THEMEOBJECT_HPP
#define THEMEOBJECT_HPP

#include "playlistthemeobject.hpp"
#include "osdthemeobject.hpp"

class ThemeObject : public QObject {
    Q_OBJECT
    THEME_C(OsdThemeObject, osd)
    THEME_C(PlaylistThemeObject, playlist)
};

#endif // THEMEOBJECT_HPP
