#ifndef SUBTITLESTYLE_H
#define SUBTITLESTYLE_H

#include "stdafx.hpp"
#include "enum/osdscalepolicy.hpp"

class Record;

struct SubtitleStyle {
    struct Font {
        using Scale = OsdScalePolicy;
        Font() { qfont.setPixelSize(height()); }
        auto family() const -> QString { return qfont.family(); }
        auto bold() const -> bool { return qfont.bold(); }
        auto italic() const -> bool { return qfont.italic(); }
        auto underline() const -> bool { return qfont.underline(); }
        auto strikeOut() const -> bool { return qfont.strikeOut(); }
        auto setFamily(const QString &family) -> void
            { qfont.setFamily(family); }
        auto setBold(bool bold) -> void { qfont.setBold(bold); }
        auto setItalic(bool italic) -> void { qfont.setItalic(italic); }
        auto setUnderline(bool underline) -> void
            { qfont.setUnderline(underline); }
        auto setStrikeOut(bool strikeOut) -> void
            { qfont.setStrikeOut(strikeOut); }
        const QFont &font() const { return qfont; }
        static constexpr auto height() -> int { return 20; }
        auto weight() const -> int { return qfont.weight(); }
        QColor color = { Qt::white };
        double size = 0.03;
        Scale scale = Scale::Width;
        QFont qfont;
    };
    struct BBox {
        bool enabled = false;
        QColor color = {0, 0, 0, 127};
        QPointF padding = {0.3, 0.1};
    };
    struct Shadow {
        bool enabled = true, blur = true;
        QColor color = {0, 0, 0, 127};
        QPointF offset = {0.1, 0.1};
    };
    struct Outline {
        QColor color = {Qt::black};
        double width = 0.05;
        bool enabled = true;
    };
    struct Spacing {
        double line = 0, paragraph = 0;
    };
    QTextOption::WrapMode wrapMode = QTextOption::WrapAtWordBoundaryOrAnywhere;
    Shadow shadow;
    Outline outline;
    Font font;
    Spacing spacing;
    BBox bbox;
    auto save(Record &r, const QString &group) const -> void;
    auto load(Record &r, const QString &group) -> void;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

#endif // SUBTITLESTYLE_H
