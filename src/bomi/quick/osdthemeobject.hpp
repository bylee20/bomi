#ifndef OSDSTYLE_HPP
#define OSDSTYLE_HPP

#include "themeobject_helper.hpp"
#include "misc/osdstyle.hpp"
#include "enum/textthemestyle.hpp"
#include "enum/verticalalignment.hpp"

struct TimelineTheme {
    bool show_on_seeking = true;
    VerticalAlignment position = VerticalAlignment::Center;
    int duration = 2500;
};

struct MessageTheme {
    bool show_on_action = true;
    bool show_on_resized = true;
    int duration = 2500;
};

struct OsdTheme {
    OsdStyle style;
    TimelineTheme timeline;
    MessageTheme message;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

Q_DECLARE_METATYPE(OsdTheme)

class TimelineThemeObject : public QObject {
    Q_OBJECT
    Q_ENUMS(Position)
    Q_PROPERTY(Position position READ position NOTIFY changed)
    Q_PROPERTY(int duration READ duration NOTIFY changed)
public:
    enum Position {
        Top = static_cast<int>(VerticalAlignment::Top),
        Center = static_cast<int>(VerticalAlignment::Center),
        Bottom = static_cast<int>(VerticalAlignment::Bottom)
    };
    auto set(const TimelineTheme &theme) -> void { m = theme; emit changed(); }
    auto duration() const -> int { return m.duration; }
    auto position() const -> Position { return (Position)m.position; }
signals:
    void changed();
private:
    TimelineTheme m;
};

class MessageThemeObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(int duration READ duration NOTIFY changed)
public:
    auto set(const MessageTheme &theme) -> void { m = theme; emit changed(); }
    auto duration() const -> int { return m.duration; }
signals:
    void changed();
private:
    MessageTheme m;
};

class OsdStyleObject : public QObject {
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

class OsdThemeObject : public QObject {
    Q_OBJECT
    THEME_P(OsdStyleObject, style)
    THEME_P(TimelineThemeObject, timeline)
    THEME_P(MessageThemeObject, message)
public:
    auto set(const OsdTheme &theme) -> void
    {
        m_style.set(theme.style);
        m_timeline.set(theme.timeline);
        m_message.set(theme.message);
    }
};

class OsdThemeWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(OsdTheme value READ value WRITE setValue)
public:
    OsdThemeWidget(QWidget *parent = nullptr);
    ~OsdThemeWidget();
    auto value() const -> OsdTheme;
    auto setValue(const OsdTheme &theme) -> void;
private:
    struct Data;
    Data *d;
};

#endif // OSDSTYLE_HPP
