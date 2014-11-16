#include "subtitlestyle.hpp"
#include "misc/record.hpp"
#include "misc/json.hpp"

#define JSON_CLASS SubtitleStyle::Font
static const auto fontIO = JIO(JE(color), JE(size), JE(scale), JE(qfont));
#undef JSON_CLASS

#define JSON_CLASS SubtitleStyle::Outline
static const auto outlineIO = JIO(JE(enabled), JE(color), JE(width));
#undef JSON_CLASS

#define JSON_CLASS SubtitleStyle::Shadow
static const auto shadowIO = JIO(JE(enabled), JE(color), JE(blur), JE(offset));
#undef JSON_CLASS

#define JSON_CLASS SubtitleStyle::Spacing
static const auto spacingIO = JIO(JE(line), JE(paragraph));
#undef JSON_CLASS

#define JSON_CLASS SubtitleStyle::BBox
static const auto bboxIO = JIO(JE(enabled), JE(color), JE(padding));
#undef JSON_CLASS

auto json_io(SubtitleStyle::Font*) { return &fontIO; }
auto json_io(SubtitleStyle::Outline*) { return &outlineIO; }
auto json_io(SubtitleStyle::Shadow*) { return &shadowIO; }
auto json_io(SubtitleStyle::Spacing*) { return &spacingIO; }
auto json_io(SubtitleStyle::BBox*) { return &bboxIO; }

#define JSON_CLASS SubtitleStyle
static const auto jio = JIO(
    JE(font),
    JE(outline),
    JE(shadow),
    JE(spacing),
    JE(bbox)
);

JSON_DECLARE_FROM_TO_FUNCTIONS

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
