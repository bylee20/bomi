#include "osdtheme.hpp"
#include "misc/json.hpp"
#include "misc/record.hpp"

#define DO(func) {\
    func(font); \
    func(scale); \
    func(underline); \
    func(bold); \
    func(strikeout); \
    func(italic); \
    func(color); \
    func(styleColor); \
    func(style); }

auto OsdTheme::save(Record &r, const QString &group) const -> void
{
    r.beginGroup(group);
#define WRITE(a) r.write(a, #a);
    DO(WRITE)
#undef WRITE
    r.endGroup();
}

auto OsdTheme::load(Record &r, const QString &group) -> void
{
    r.beginGroup(group);
#define READ(a) r.read(a, #a);
    DO(READ)
#undef READ
    r.endGroup();
}

#define JSON_CLASS OsdTheme

static const auto jio = JIO(
    JE(font),
    JE(scale),
    JE(underline),
    JE(bold),
    JE(strikeout),
    JE(italic),
    JE(color),
    JE(styleColor),
    JE(style)
);

JSON_DECLARE_FROM_TO_FUNCTIONS

/******************************************************************************/

OsdThemeObject::OsdThemeObject(QObject *parent)
    : QObject(parent)
{

}
