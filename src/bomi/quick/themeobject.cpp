#include "themeobject.hpp"
#include "player/app.hpp"
#include <QQuickItem>

auto ThemeObject::monospace() const -> QString
{
    return cApp.fixedFont().family();
}

auto ThemeObject::font() const -> QFont
{
    return cApp.font();
}
