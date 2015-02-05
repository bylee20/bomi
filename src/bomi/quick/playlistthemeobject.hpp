#ifndef PLAYLISTTHEMEOBJECT_HPP
#define PLAYLISTTHEMEOBJECT_HPP

#include <QObject>
#include "themeobject_helper.hpp"

struct PlaylistTheme {
    bool showLocation = true;
    bool showOnMouseOverEdge = true;
    DECL_EQ(PlaylistTheme, &T::showLocation, &T::showOnMouseOverEdge)
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

Q_DECLARE_METATYPE(PlaylistTheme);

class PlaylistThemeObject : public QObject {
    Q_OBJECT
    THEME_P(bool, showLocation)
    THEME_P(bool, showOnMouseOverEdge)
public:
    PlaylistTheme m;
};

class PlaylistThemeWidget : public QGroupBox {
    Q_OBJECT
    Q_PROPERTY(PlaylistTheme value READ value WRITE setValue)
public:
    PlaylistThemeWidget(QWidget *parent = nullptr);
    auto value() const -> PlaylistTheme;
    auto setValue(const PlaylistTheme &theme) -> void;
private:
    QCheckBox *m_location, *m_edge;
};

/******************************************************************************/

struct HistoryTheme {
    bool showOnMouseOverEdge = true;
    DECL_EQ(HistoryTheme, &T::showOnMouseOverEdge)
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

Q_DECLARE_METATYPE(HistoryTheme);

class HistoryThemeObject : public QObject {
    Q_OBJECT
    THEME_P(bool, showOnMouseOverEdge)
public:
    HistoryTheme m;
};

class HistoryThemeWidget: public QGroupBox {
    Q_OBJECT
    Q_PROPERTY(HistoryTheme value READ value WRITE setValue)
public:
    HistoryThemeWidget(QWidget *parent = nullptr);
    auto value() const -> HistoryTheme;
    auto setValue(const HistoryTheme &theme) -> void;
private:
    QCheckBox *m_edge;
};

#endif // PLAYLISTTHEMEOBJECT_HPP
