#ifndef OSDSTYLE_HPP
#define OSDSTYLE_HPP

#include "themeobject_helper.hpp"
#include "misc/osdstyle.hpp"
#include "enum/textthemestyle.hpp"
#include "enum/verticalalignment.hpp"

struct TimelineTheme {
    bool show_on_seeking = true;
    VerticalAlignment position = VerticalAlignment::Center;
    double margin = 0.1;
    int duration = 2500;
    DECL_EQ(TimelineTheme, &T::show_on_seeking, &T::position, &T::margin, &T::duration)
};

struct MessageTheme {
    bool show_on_action = true;
    bool show_on_resized = true;
    int duration = 2500;
    DECL_EQ(MessageTheme, &T::show_on_action, &T::show_on_resized, &T::duration)
};

struct OsdTheme {
    OsdStyle style;
    TimelineTheme timeline;
    MessageTheme message;
    DECL_EQ(OsdTheme, &T::style, &T::timeline, &T::message)
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

Q_DECLARE_METATYPE(OsdTheme)

class TimelineThemeObject : public QObject {
public:
    enum Position {
        Top = static_cast<int>(VerticalAlignment::Top),
        Center = static_cast<int>(VerticalAlignment::Center),
        Bottom = static_cast<int>(VerticalAlignment::Bottom)
    };
private:
    Q_OBJECT
    Q_ENUMS(Position)
    THEME_PV(Position, position, (Position)m.position)
    THEME_P(qreal, margin)
    THEME_P(int, duration)
public:
    TimelineTheme m;
};

class MessageThemeObject : public QObject {
    Q_OBJECT
    THEME_P(int, duration)
public:
    MessageTheme m;
};

class OsdStyleObject : public QObject {
public:
    enum Style {
        Normal  = static_cast<int>(TextThemeStyle::Normal),
        Outline = static_cast<int>(TextThemeStyle::Outline),
        Raised  = static_cast<int>(TextThemeStyle::Raised),
        Sunked  = static_cast<int>(TextThemeStyle::Sunken)
    };
private:
    Q_OBJECT
    Q_ENUMS(Style)
    THEME_PV(QString, font, m.font.family())
    THEME_PV(qreal, scale, m.font.size)
    THEME_PV(bool, underline, m.font.underline())
    THEME_PV(bool, bold, m.font.bold())
    THEME_PV(bool, strikeout, m.font.strikeOut())
    THEME_PV(bool, italic, m.font.italic())
    THEME_PV(Style, style, Outline)
    THEME_PV(QColor, color, m.font.color)
    THEME_PV(QColor, styleColor, m.outline.color)
public:
    OsdStyle m;
};

class OsdThemeObject : public QObject {
    Q_OBJECT
public:
    struct {
        OsdStyleObject style;
        TimelineThemeObject timeline;
        MessageThemeObject message;
    } mutable m;
    THEME_PV(OsdStyleObject*, style, &m.style)
    THEME_PV(TimelineThemeObject*, timeline, &m.timeline)
    THEME_PV(MessageThemeObject*, message, &m.message)
};

class OsdThemeWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(OsdTheme value READ value WRITE setValue NOTIFY valueChanged)
public:
    OsdThemeWidget(QWidget *parent = nullptr);
    ~OsdThemeWidget();
    auto value() const -> OsdTheme;
    auto setValue(const OsdTheme &theme) -> void;
signals:
    void valueChanged();
private:
    auto emitChanged();
    struct Data;
    Data *d;
};

#endif // OSDSTYLE_HPP
