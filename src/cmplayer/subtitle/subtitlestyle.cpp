#include "subtitlestyle.hpp"
#include "misc/record.hpp"
#include "misc/json.hpp"

auto SubtitleStyle::toJson() const -> QJsonObject
{
    JsonObject json;
    json["font.color"] = font.color;
    json["font.size"] = font.size;
    json["font.scale"] = font.scale;
    json["font.qfont"] = font.qfont;
    json["outline.enabled"] = outline.enabled;
    json["outline.color"] = outline.color;
    json["outline.width"] = outline.width;
    json["shadow.enabled"] = shadow.enabled;
    json["shadow.color"] = shadow.color;
    json["shadow.blur"] = shadow.blur;
    json["shadow.offset"] = shadow.offset;
    json["spacing.line"] = spacing.line;
    json["spacing.paragraph"] = spacing.paragraph;
    json["bbox.enabled"] = bbox.enabled;
    json["bbox.color"] = bbox.color;
    json["bbox.padding"] = bbox.padding;
    return json.qt();
}

auto SubtitleStyle::fromJson(const QJsonObject &qjson) -> SubtitleStyle
{
    SubtitleStyle s;
    const JsonObject json(qjson);
    json.get("font.color", s.font.color);
    json.get("font.size", s.font.size);
    json.get("font.scale", s.font.scale);
    json.get("font.qfont", s.font.qfont);
    json.get("outline.enabled", s.outline.enabled);
    json.get("outline.color", s.outline.color);
    json.get("outline.width", s.outline.width);
    json.get("shadow.enabled", s.shadow.enabled);
    json.get("shadow.color", s.shadow.color);
    json.get("shadow.blur", s.shadow.blur);
    json.get("shadow.offset", s.shadow.offset);
    json.get("spacing.line", s.spacing.line);
    json.get("spacing.paragraph", s.spacing.paragraph);
    json.get("bbox.enabled", s.bbox.enabled);
    json.get("bbox.color", s.bbox.color);
    json.get("bbox.padding", s.bbox.padding);
    return s;
}

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
