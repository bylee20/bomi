#ifndef PLAYLISTTHEMEOBJECT_HPP
#define PLAYLISTTHEMEOBJECT_HPP

#include <QObject>
#include "themeobject_helper.hpp"

struct ControlsTheme {
    bool showOnMouseMoved = true;
    bool showLocationsInPlaylist = true;
    bool showPlaylistOnMouseOverEdge = true;
    bool showHistoryOnMouseOverEdge = true;
    bool showPreviewOnMouseOverSeekBar = false;
    DECL_EQ(ControlsTheme, &T::showOnMouseMoved, &T::showLocationsInPlaylist,
            &T::showPlaylistOnMouseOverEdge, &T::showHistoryOnMouseOverEdge,
            &T::showPreviewOnMouseOverSeekBar)
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

Q_DECLARE_METATYPE(ControlsTheme);

class ControlsThemeObject : public QObject {
    Q_OBJECT
    THEME_P(bool, showOnMouseMoved)
    THEME_P(bool, showLocationsInPlaylist)
    THEME_P(bool, showPlaylistOnMouseOverEdge)
    THEME_P(bool, showHistoryOnMouseOverEdge)
    THEME_P(bool, showPreviewOnMouseOverSeekBar)
public:
    ControlsTheme m;
};

class ControlsThemeWidget : public QGroupBox {
    Q_OBJECT
    Q_PROPERTY(ControlsTheme value READ value WRITE setValue NOTIFY valueChanged)
public:
    ControlsThemeWidget(QWidget *parent = nullptr);
    ~ControlsThemeWidget();
    auto value() const -> ControlsTheme;
    auto setValue(const ControlsTheme &theme) -> void;
signals:
    void valueChanged();
private:
    struct Data;
    Data *d;
};

struct PlaylistTheme {
    bool showLocation = true;
    bool showOnMouseOverEdge = true;
    DECL_EQ(PlaylistTheme, &T::showLocation, &T::showOnMouseOverEdge)
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

Q_DECLARE_METATYPE(PlaylistTheme);

struct HistoryTheme {
    bool showOnMouseOverEdge = true;
    DECL_EQ(HistoryTheme, &T::showOnMouseOverEdge)
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

Q_DECLARE_METATYPE(HistoryTheme);

#endif // PLAYLISTTHEMEOBJECT_HPP
