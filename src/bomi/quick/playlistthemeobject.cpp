#include "playlistthemeobject.hpp"
#include "misc/json.hpp"

#define JSON_CLASS PlaylistTheme
static const auto jioPlaylist = JIO(
    JE(showLocation),
    JE(showOnMouseOverEdge)
);

JSON_DECLARE_FROM_TO_FUNCTIONS_IO(jioPlaylist)

#undef JSON_CLASS

#define JSON_CLASS HistoryTheme
static const auto jio = JIO(
    JE(showOnMouseOverEdge)
);

JSON_DECLARE_FROM_TO_FUNCTIONS

/******************************************************************************/

PlaylistThemeWidget::PlaylistThemeWidget(QWidget *parent)
    : QGroupBox(parent)
{
    m_location = new QCheckBox(tr("Show location in playlist"));
    m_edge = new QCheckBox(tr("Show playlist when mouse hovers on the right edge"));
    auto vbox = new QVBoxLayout;
    vbox->addWidget(m_location);
    vbox->addWidget(m_edge);
    setLayout(vbox);

    auto signal = &PlaylistThemeWidget::valueChanged;
    PLUG_CHANGED(m_location);
    PLUG_CHANGED(m_edge);
}
auto PlaylistThemeWidget::value() const -> PlaylistTheme
{
    PlaylistTheme theme;
    theme.showLocation = m_location->isChecked();
    theme.showOnMouseOverEdge = m_edge->isChecked();
    return theme;
}
auto PlaylistThemeWidget::setValue(const PlaylistTheme &theme) -> void
{
    m_location->setChecked(theme.showLocation);
    m_edge->setChecked(theme.showOnMouseOverEdge);
}

/******************************************************************************/

HistoryThemeWidget::HistoryThemeWidget(QWidget *parent)
    : QGroupBox(parent)
{
    m_edge = new QCheckBox(tr("Show history when mouse hovers on the left edge"));
    auto vbox = new QVBoxLayout;
    vbox->addWidget(m_edge);
    setLayout(vbox);

    auto signal = &HistoryThemeWidget::valueChanged;
    PLUG_CHANGED(m_edge);
}

auto HistoryThemeWidget::value() const -> HistoryTheme
{
    HistoryTheme theme;
    theme.showOnMouseOverEdge = m_edge->isChecked();
    return theme;
}

auto HistoryThemeWidget::setValue(const HistoryTheme &theme) -> void
{
    m_edge->setChecked(theme.showOnMouseOverEdge);
}
