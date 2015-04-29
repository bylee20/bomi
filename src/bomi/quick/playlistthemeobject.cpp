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
    JE(showMediaTitleForUrlsInHistory),
    JE(previewSize),
    JE(previewMinimumSize),
    JE(previewMaximumSize)
);

JSON_DECLARE_FROM_TO_FUNCTIONS

#undef JSON_CLASS

/******************************************************************************/

struct ControlsThemeWidget::Data {
    Ui::ControlsThemeWidget ui;
    QTreeWidgetItem *titleBarEnabled, *showOnMouseMoved,
        *showLocationsInPlaylist, *showToolOnMouseOverEdge,
        *showPreviewOnMouseOverSeekBar, *showKeyframeForPreview,
        *showMediaTitleForLocalFilesInHistory, *showMediaTitleForUrlsInHistory,
        *previewSize;
    auto addItem(const QString &text,
                 QTreeWidgetItem *parent = nullptr) -> QTreeWidgetItem*
    {
        auto item = new QTreeWidgetItem;
        item->setText(0, text);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(0, Qt::Unchecked);
        if (parent)
            parent->addChild(item);
        else
            ui.tree->addTopLevelItem(item);
        return item;
    }
};

ControlsThemeWidget::ControlsThemeWidget(QWidget *parent)
    : QGroupBox(parent), d(new Data)
{
    d->ui.setupUi(this);
    d->titleBarEnabled = d->addItem(tr("Enable internal title bar in frameless mode"));
    d->showOnMouseMoved = d->addItem(tr("Show hidden controls whenever mouse moved"));
    d->showLocationsInPlaylist = d->addItem(tr("Show locations in playlist"));
    d->showMediaTitleForLocalFilesInHistory = d->addItem(tr("Show media title for local files in name column of history"));
    d->showMediaTitleForUrlsInHistory = d->addItem(tr("Show media title for remote URLs in name column of history"));
    d->showToolOnMouseOverEdge = d->addItem(tr("Show history/playlist when mouse hovers on screen edge"));
    d->showPreviewOnMouseOverSeekBar = d->addItem(tr("Show preview when mouse hovers on seek bar"));
    d->showKeyframeForPreview = d->addItem(tr("Show nearest keyframe instead of exact frame"), d->showPreviewOnMouseOverSeekBar);
    d->previewSize = new QTreeWidgetItem(d->showPreviewOnMouseOverSeekBar);
    d->previewSize->setFlags(Qt::ItemIsEnabled);
    d->ui.tree->setItemWidget(d->previewSize, 0, d->ui.preview_size_widget);

    d->ui.tree->expandAll();

    auto signal = &ControlsThemeWidget::valueChanged;
    connect(d->ui.tree, &QTreeWidget::itemChanged, this, [=] (QTreeWidgetItem *item) {
        if (item == d->showPreviewOnMouseOverSeekBar) {
            const bool preview = item->checkState(0);
            for (int i = 0; i < item->childCount(); ++i) {
                auto item = d->showPreviewOnMouseOverSeekBar->child(i);
                auto flags = item->flags();
                if (preview)
                    flags |= Qt::ItemIsEnabled;
                else
                    flags &= ~Qt::ItemIsEnabled;
                item->setFlags(flags);
            }
            d->ui.preview_size_widget->setEnabled(preview);
        }
        emit valueChanged();
    });

    PLUG_CHANGED(d->ui.preview_size);
    PLUG_CHANGED(d->ui.preview_min);
    PLUG_CHANGED(d->ui.preview_max);
}

ControlsThemeWidget::~ControlsThemeWidget()
{
    delete d;
}

#define CHECK_LIST \
   {CHECK(titleBarEnabled); \
    CHECK(showOnMouseMoved); \
    CHECK(showLocationsInPlaylist); \
    CHECK(showToolOnMouseOverEdge); \
    CHECK(showPreviewOnMouseOverSeekBar); \
    CHECK(showKeyframeForPreview); \
    CHECK(showMediaTitleForLocalFilesInHistory); \
    CHECK(showMediaTitleForUrlsInHistory);}

auto ControlsThemeWidget::value() const -> ControlsTheme
{
    ControlsTheme theme;
#define CHECK(v) {theme.v = d->v->checkState(0);}
    CHECK_LIST;
#undef CHECK
    theme.previewSize = d->ui.preview_size->value() * 1e-2;
    theme.previewMinimumSize = d->ui.preview_min->value();
    theme.previewMaximumSize = d->ui.preview_max->value();
    return theme;
}

auto ControlsThemeWidget::setValue(const ControlsTheme &theme) -> void
{
#define CHECK(v) {d->v->setCheckState(0, theme.v ? Qt::Checked : Qt::Unchecked);}
    CHECK_LIST;
#undef CHECK
    d->ui.preview_size->setValue(theme.previewSize * 1e2 + 0.5);
    d->ui.preview_min->setValue(theme.previewMinimumSize);
    d->ui.preview_max->setValue(theme.previewMaximumSize);
}

/******************************************************************************/

#define JSON_CLASS PlaylistTheme
static const auto jioPlaylist = JIO(
    JE(showLocation),
    JE(showOnMouseOverEdge)
);

JSON_DECLARE_FROM_TO_FUNCTIONS_IO(jioPlaylist)

#undef JSON_CLASS
