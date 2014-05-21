#include "subtitlestyle.hpp"
#include "misc/record.hpp"

auto SubtitleStyle::save(Record &r, const QString &group) const -> void
{
    r.beginGroup(group);
#define WRITE(a) r.write(a, #a)
    WRITE(font.color);
    WRITE(font.size);
    WRITE(font.scale);
    WRITE(font.qfont);
    WRITE(outline.enabled);
    WRITE(outline.color);
    WRITE(outline.width);
    WRITE(shadow.enabled);
    WRITE(shadow.color);
    WRITE(shadow.blur);
    WRITE(shadow.offset);
    WRITE(spacing.line);
    WRITE(spacing.paragraph);
    WRITE(bbox.enabled);
    WRITE(bbox.color);
    WRITE(bbox.padding);
#undef WRITE
    r.endGroup();
}

auto SubtitleStyle::load(Record &r, const QString &group) -> void
{
    r.beginGroup(group);
#define READ(a) r.read(a, #a)
    READ(font.color);
    READ(font.size);
    READ(font.scale);
    READ(font.qfont);
    READ(outline.enabled);
    READ(outline.color);
    READ(outline.width);
    READ(shadow.enabled);
    READ(shadow.color);
    READ(shadow.blur);
    READ(shadow.offset);
    READ(spacing.line);
    READ(spacing.paragraph);
    READ(bbox.enabled);
    READ(bbox.color);
    READ(bbox.padding);
#undef READ
    r.endGroup();
}
