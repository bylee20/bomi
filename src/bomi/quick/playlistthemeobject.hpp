#ifndef PLAYLISTTHEMEOBJECT_HPP
#define PLAYLISTTHEMEOBJECT_HPP

#include <QObject>
#include "themeobject_helper.hpp"

struct ControlsTheme {
    bool titleBarEnabled = true;
    bool showOnMouseMoved = true;
    bool showLocationsInPlaylist = true;
    bool showToolOnMouseOverEdge = false;
    bool showPreviewOnMouseOverSeekBar = false;
    bool showKeyframeForPreview = true;
    bool showMediaTitleForLocalFilesInHistory = false;
    bool showMediaTitleForUrlsInHistory = true;
    qreal previewSize = 0.2;
    int previewMinimumSize = 100;
    int previewMaximumSize = 200;
    DECL_EQ(ControlsTheme, &T::showOnMouseMoved, &T::showLocationsInPlaylist,
            &T::showToolOnMouseOverEdge, &T::showPreviewOnMouseOverSeekBar,
            &T::showMediaTitleForUrlsInHistory, &T::showKeyframeForPreview,
            &T::showMediaTitleForLocalFilesInHistory, &T::titleBarEnabled,
            &T::previewSize, &T::previewMinimumSize, &T::previewMaximumSize)
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

Q_DECLARE_METATYPE(ControlsTheme);

class ControlsThemeObject : public QObject {
    Q_OBJECT
    THEME_P(bool, titleBarEnabled)
    THEME_P(bool, showOnMouseMoved)
    THEME_P(bool, showLocationsInPlaylist)
    THEME_P(bool, showToolOnMouseOverEdge)
    THEME_P(bool, showPreviewOnMouseOverSeekBar)
    THEME_P(bool, showKeyframeForPreview)
    THEME_P(qreal, previewSize)
    THEME_P(int, previewMinimumSize)
    THEME_P(int, previewMaximumSize)
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

#endif // PLAYLISTTHEMEOBJECT_HPP
