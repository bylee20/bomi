#ifndef OSDSTYLE_HPP
#define OSDSTYLE_HPP

#include "misc/osdstyle.hpp"
#include "enum/textthemestyle.hpp"

struct OsdTheme : public OsdStyle {

};

class OsdThemeObject : public QObject {
    Q_OBJECT
    Q_ENUMS(Style)
    Q_PROPERTY(QString font READ font NOTIFY changed)
    Q_PROPERTY(qreal scale READ scale NOTIFY changed)
    Q_PROPERTY(bool underline READ underline NOTIFY changed)
    Q_PROPERTY(bool bold READ bold NOTIFY changed)
    Q_PROPERTY(bool strikeout READ strikeout NOTIFY changed)
    Q_PROPERTY(bool italic READ italic NOTIFY changed)
    Q_PROPERTY(Style style READ style NOTIFY changed)
    Q_PROPERTY(QColor color READ color NOTIFY changed)
    Q_PROPERTY(QColor styleColor READ styleColor NOTIFY changed)
public:
    enum Style {
        Normal  = static_cast<int>(TextThemeStyle::Normal),
        Outline = static_cast<int>(TextThemeStyle::Outline),
        Raised  = static_cast<int>(TextThemeStyle::Raised),
        Sunked  = static_cast<int>(TextThemeStyle::Sunken)
    };
    auto set(const OsdStyle &theme) -> void { m = theme; emit changed(); }
    auto font() const -> QString { return m.font.family(); }
    auto underline() const -> bool { return m.font.underline(); }
    auto strikeout() const -> bool { return m.font.strikeOut(); }
    auto bold() const -> bool { return m.font.bold(); }
    auto italic() const -> bool { return m.font.italic(); }
    auto color() const -> QColor { return m.font.color; }
    auto styleColor() const -> QColor { return m.outline.color; }
    auto style() const -> Style { return Outline; }
    auto scale() const -> qreal { return m.font.size; }
signals:
    void changed();
private:
    OsdStyle m;
};

#endif // OSDSTYLE_HPP
