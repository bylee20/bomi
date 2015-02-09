#include "osdthemeobject.hpp"
#include "misc/json.hpp"

#define JSON_CLASS TimelineTheme
static const auto timelineIO = JIO(
    JE(show_on_seeking), JE(position), JE(margin), JE(duration)
);
#undef JSON_CLASS

#define JSON_CLASS MessageTheme
static const auto messageIO = JIO(JE(show_on_action), JE(show_on_resized), JE(duration));
#undef JSON_CLASS

auto json_io(TimelineTheme*) { return &timelineIO; }
auto json_io(MessageTheme*) { return &messageIO; }

#define JSON_CLASS OsdTheme
static const auto jio = JIO(
    JE(style),
    JE(timeline),
    JE(message)
);

JSON_DECLARE_FROM_TO_FUNCTIONS

#include "ui_osdthemewidget.h"

struct OsdThemeWidget::Data {
    Ui::OsdThemeWidget ui;
};

OsdThemeWidget::OsdThemeWidget(QWidget *parent)
    : QWidget(parent), d(new Data)
{
    d->ui.setupUi(this);
    d->ui.style->setShadowVsible(false);
    d->ui.style->setSpacingVisible(false);
    d->ui.style->setBBoxVisible(false);

    connect(d->ui.timeline_position, &VerticalAlignmentComboBox::currentDataChanged,
            this, [=] () {
        const bool center = d->ui.timeline_position->currentEnum() == VerticalAlignment::Center;
        d->ui.timeline_margin->setEnabled(!center);
    });

    auto signal = &OsdThemeWidget::valueChanged;
    PLUG_CHANGED(d->ui.style);
    PLUG_CHANGED(d->ui.timeline_visible);
    PLUG_CHANGED(d->ui.timeline_position);
    PLUG_CHANGED(d->ui.timeline_margin);
    PLUG_CHANGED(d->ui.timeline_duration);
    PLUG_CHANGED(d->ui.message_on_action);
    PLUG_CHANGED(d->ui.message_on_resized);
    PLUG_CHANGED(d->ui.message_duration);
}

OsdThemeWidget::~OsdThemeWidget()
{
    delete d;
}

auto OsdThemeWidget::value() const -> OsdTheme
{
    OsdTheme theme;
    theme.style = d->ui.style->value();
    theme.timeline.show_on_seeking = d->ui.timeline_visible->isChecked();
    theme.timeline.position = d->ui.timeline_position->currentEnum();
    theme.timeline.margin = d->ui.timeline_margin->value()/100.0;
    theme.timeline.duration = d->ui.timeline_duration->value() * 1000 + 0.5;
    theme.message.show_on_action = d->ui.message_on_action->isChecked();
    theme.message.show_on_resized = d->ui.message_on_resized->isChecked();
    theme.message.duration = d->ui.message_duration->value() * 1000 + 0.5;
    return theme;
}

auto OsdThemeWidget::setValue(const OsdTheme &theme) -> void
{
    d->ui.style->setValue(theme.style);
    d->ui.timeline_visible->setChecked(theme.timeline.show_on_seeking);
    d->ui.timeline_position->setCurrentEnum(theme.timeline.position);
    d->ui.timeline_margin->setValue(theme.timeline.margin * 100 + 0.5);
    d->ui.timeline_duration->setValue(theme.timeline.duration/1e3);
    d->ui.message_on_action->setChecked(theme.message.show_on_action);
    d->ui.message_on_resized->setChecked(theme.message.show_on_resized);
    d->ui.message_duration->setValue(theme.message.duration/1e3);
}
