#include "osdtheme.hpp"
#include "record.hpp"

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

OsdThemeObject::OsdThemeObject(QObject *parent)
    : QObject(parent)
{

}
