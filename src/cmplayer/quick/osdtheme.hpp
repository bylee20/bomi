#ifndef OSDSTYLE_HPP
#define OSDSTYLE_HPP

#include "stdafx.hpp"
#include "enum/textthemestyle.hpp"

class Record;

struct OsdTheme {
    QString font = qApp->font().family();
    qreal scale = 0.025;
    bool underline = false, bold = false;
    bool strikeout = false, italic = false;
    TextThemeStyle style;
    QColor color = Qt::white, styleColor = Qt::black;
    auto save(Record &r, const QString &group) const -> void;
    auto load(Record &r, const QString &group) -> void;
    auto toJson() const -> QJsonObject;
    static auto fromJson(const QJsonObject &json) -> OsdTheme;
};

/******************************************************************************/

class OsdThemeObject : public QObject {
    Q_OBJECT
    Q_ENUMS(Style)
    Q_PROPERTY(QString font READ font CONSTANT FINAL)
    Q_PROPERTY(qreal scale READ scale CONSTANT FINAL)
    Q_PROPERTY(bool underline READ underline CONSTANT FINAL)
    Q_PROPERTY(bool bold READ bold CONSTANT FINAL)
    Q_PROPERTY(bool strikeout READ strikeout CONSTANT FINAL)
    Q_PROPERTY(bool italic READ italic CONSTANT FINAL)
    Q_PROPERTY(Style style READ style CONSTANT FINAL)
    Q_PROPERTY(QColor color READ color CONSTANT FINAL)
    Q_PROPERTY(QColor styleColor READ styleColor CONSTANT FINAL)
public:
    enum Style {
        Normal  = static_cast<int>(TextThemeStyle::Normal),
        Outline = static_cast<int>(TextThemeStyle::Outline),
        Raised  = static_cast<int>(TextThemeStyle::Raised),
        Sunked  = static_cast<int>(TextThemeStyle::Sunken)
    };
    OsdThemeObject(QObject *parent = nullptr);
    auto set(const OsdTheme &theme) -> void { m = theme; }
    auto font() const -> QString { return m.font; }
    auto underline() const -> bool { return m.underline; }
    auto strikeout() const -> bool { return m.strikeout; }
    auto bold() const -> bool { return m.bold; }
    auto italic() const -> bool { return m.italic; }
    auto color() const -> QColor { return m.color; }
    auto styleColor() const -> QColor { return m.styleColor; }
    auto style() const -> Style { return static_cast<Style>(m.style); }
    auto scale() const -> qreal { return m.scale; }
private:
    OsdTheme m;
};

#endif // OSDSTYLE_HPP
