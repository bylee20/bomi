#ifndef THEMEOBJECT_HPP
#define THEMEOBJECT_HPP

#include "playlistthemeobject.hpp"
#include "osdthemeobject.hpp"

class ThemeObject : public QObject {
    Q_OBJECT
    THEME_P(OsdThemeObject, osd)
    THEME_P(PlaylistThemeObject, playlist)
};

#endif // THEMEOBJECT_HPP
