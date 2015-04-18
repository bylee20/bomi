#include "playlistthemeobject.hpp"
#include "misc/json.hpp"
#include "ui_controlsthemewidget.h"

#define JSON_CLASS ControlsTheme

static const auto jio = JIO(
    JE(titleBarEnabled),
    JE(showOnMouseMoved),
    JE(showLocationsInPlaylist),
    JE(showToolOnMouseOverEdge),
    JE(showPreviewOnMouseOverSeekBar),
    JE(showKeyframeForPreview),
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
    PLUG_CHANGED(d->ui.enable_title_bar);
    PLUG_CHANGED(d->ui.show_hidden_on_moved);
    PLUG_CHANGED(d->ui.show_locations);
    PLUG_CHANGED(d->ui.show_tool_on_hovered);
    PLUG_CHANGED(d->ui.show_preview);
    PLUG_CHANGED(d->ui.show_preview_keyframe);
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
    theme.titleBarEnabled = d->ui.enable_title_bar->isChecked();
    theme.showOnMouseMoved = d->ui.show_hidden_on_moved->isChecked();
    theme.showLocationsInPlaylist = d->ui.show_locations->isChecked();
    theme.showToolOnMouseOverEdge = d->ui.show_tool_on_hovered->isChecked();
    theme.showPreviewOnMouseOverSeekBar = d->ui.show_preview->isChecked();
    theme.showKeyframeForPreview = d->ui.show_preview_keyframe->isChecked();
    theme.showMediaTitleForLocalFilesInHistory = d->ui.show_media_title_local_files->isChecked();
    theme.showMediaTitleForUrlsInHistory = d->ui.show_media_title_urls->isChecked();
    return theme;
}

auto ControlsThemeWidget::setValue(const ControlsTheme &theme) -> void
{
    d->ui.enable_title_bar->setChecked(theme.titleBarEnabled);
    d->ui.show_hidden_on_moved->setChecked(theme.showOnMouseMoved);
    d->ui.show_locations->setChecked(theme.showLocationsInPlaylist);
    d->ui.show_tool_on_hovered->setChecked(theme.showToolOnMouseOverEdge);
    d->ui.show_preview->setChecked(theme.showPreviewOnMouseOverSeekBar);
    d->ui.show_preview_keyframe->setChecked(theme.showKeyframeForPreview);
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
