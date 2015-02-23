#include "themeobject.hpp"
#include <QQuickItem>

auto ThemeObject::monospace() const -> QString
{
    return u"monospace"_q;
}

auto ThemeObject::font() const -> QFont
{
    return qApp->font();
}
