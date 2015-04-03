#include "playlistthemeobject.hpp"
#include "misc/json.hpp"
#include "ui_controlsthemewidget.h"

#define JSON_CLASS ControlsTheme

static const auto jio = JIO(
    JE(showOnMouseMoved),
    JE(showLocationsInPlaylist),
    JE(showPlaylistOnMouseOverEdge),
    JE(showHistoryOnMouseOverEdge),
    JE(showPreviewOnMouseOverSeekBar),
    JE(showMediaTitleForLocalFilesInHistory),
    JE(showMediaTitleForUrlsInHistory)
);

JSON_DECLARE_FROM_TO_FUNCTIONS

#undef JSON_CLASS

/******************************************************************************/

struct ControlsThemeWidget::Data {
    Ui::ControlsThemeWidget ui;
};

ControlsThemeWidget::ControlsThemeWidget(QWidget *parent)
    : QGroupBox(parent), d(new Data)
{
    d->ui.setupUi(this);
    auto signal = &ControlsThemeWidget::valueChanged;
    PLUG_CHANGED(d->ui.show_hidden_on_moved);
    PLUG_CHANGED(d->ui.show_locations);
    PLUG_CHANGED(d->ui.show_playlist_on_hovered);
    PLUG_CHANGED(d->ui.show_history_on_hovered);
    PLUG_CHANGED(d->ui.show_preview);
    PLUG_CHANGED(d->ui.show_media_title_urls);
    PLUG_CHANGED(d->ui.show_media_title_local_files);
}

ControlsThemeWidget::~ControlsThemeWidget()
{
    delete d;
}

auto ControlsThemeWidget::value() const -> ControlsTheme
{
    ControlsTheme theme;
    theme.showOnMouseMoved = d->ui.show_hidden_on_moved->isChecked();
    theme.showLocationsInPlaylist = d->ui.show_locations->isChecked();
    theme.showPlaylistOnMouseOverEdge = d->ui.show_playlist_on_hovered->isChecked();
    theme.showHistoryOnMouseOverEdge = d->ui.show_history_on_hovered->isChecked();
    theme.showPreviewOnMouseOverSeekBar = d->ui.show_preview->isChecked();
    theme.showMediaTitleForLocalFilesInHistory = d->ui.show_media_title_local_files->isChecked();
    theme.showMediaTitleForUrlsInHistory = d->ui.show_media_title_urls->isChecked();
    return theme;
}

auto ControlsThemeWidget::setValue(const ControlsTheme &theme) -> void
{
    d->ui.show_hidden_on_moved->setChecked(theme.showOnMouseMoved);
    d->ui.show_locations->setChecked(theme.showLocationsInPlaylist);
    d->ui.show_playlist_on_hovered->setChecked(theme.showPlaylistOnMouseOverEdge);
    d->ui.show_history_on_hovered->setChecked(theme.showHistoryOnMouseOverEdge);
    d->ui.show_preview->setChecked(theme.showPreviewOnMouseOverSeekBar);
    d->ui.show_media_title_local_files->setChecked(theme.showMediaTitleForLocalFilesInHistory);
    d->ui.show_media_title_urls->setChecked(theme.showMediaTitleForUrlsInHistory);
}

/******************************************************************************/

#define JSON_CLASS PlaylistTheme
static const auto jioPlaylist = JIO(
    JE(showLocation),
    JE(showOnMouseOverEdge)
);

JSON_DECLARE_FROM_TO_FUNCTIONS_IO(jioPlaylist)

#undef JSON_CLASS

#define JSON_CLASS HistoryTheme
static const auto jioHistory = JIO(
    JE(showOnMouseOverEdge)
);

JSON_DECLARE_FROM_TO_FUNCTIONS_IO(jioHistory)
