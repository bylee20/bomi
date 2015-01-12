#include "osdtheme.hpp"
#include "misc/json.hpp"

#define JSON_CLASS OsdTheme::Font
static const auto fontIO = JIO(JE(color), JE(size), JE(qfont));
#undef JSON_CLASS

#define JSON_CLASS OsdTheme::Outline
static const auto outlineIO = JIO(JE(enabled), JE(color), JE(width));
#undef JSON_CLASS

#define JSON_CLASS OsdTheme::Shadow
static const auto shadowIO = JIO(JE(enabled), JE(color), JE(blur), JE(offset));
#undef JSON_CLASS

#define JSON_CLASS OsdTheme::Spacing
static const auto spacingIO = JIO(JE(line), JE(paragraph));
#undef JSON_CLASS

#define JSON_CLASS OsdTheme::BBox
static const auto bboxIO = JIO(JE(enabled), JE(color), JE(padding));
#undef JSON_CLASS

auto json_io(OsdTheme::Font*) { return &fontIO; }
auto json_io(OsdTheme::Outline*) { return &outlineIO; }
auto json_io(OsdTheme::Shadow*) { return &shadowIO; }
auto json_io(OsdTheme::Spacing*) { return &spacingIO; }
auto json_io(OsdTheme::BBox*) { return &bboxIO; }

#define JSON_CLASS OsdTheme
static const auto jio = JIO(
    JE(font),
    JE(outline),
    JE(shadow),
    JE(spacing),
    JE(bbox)
);

JSON_DECLARE_FROM_TO_FUNCTIONS

/******************************************************************************/

#include "ui_osdthemewidget.h"

struct OsdThemeWidget::Data {
    Ui::OsdThemeWidget ui;
};

OsdThemeWidget::OsdThemeWidget(QWidget *parent)
    : QWidget(parent), d(new Data)
{
    d->ui.setupUi(this);
    d->ui.font_color->setAlphaChannel(false);
    d->ui.outline_color->setAlphaChannel(false);
    d->ui.shadow_color->setAlphaChannel(true);
    d->ui.bbox_color->setAlphaChannel(true);
}

OsdThemeWidget::~OsdThemeWidget()
{
    delete d;
}

auto OsdThemeWidget::setValue(const OsdTheme &v) -> void
{
    d->ui.font_family->setCurrentFont(v.font.family());
    d->ui.font_option->set(v.font.qfont);
    d->ui.font_color->setColor(v.font.color);
    d->ui.outline->setChecked(v.outline.enabled);
    d->ui.outline_color->setColor(v.outline.color);
    d->ui.outline_width->setValue(v.outline.width*100.0);
//    d->ui.font_scale->setCurrentValue(v.font.scale);
    d->ui.font_size->setValue(v.font.size*100.0);
    d->ui.shadow->setChecked(v.shadow.enabled);
    d->ui.shadow_color->setColor(v.shadow.color);
    d->ui.shadow_offset_x->setValue(v.shadow.offset.x()*100.0);
    d->ui.shadow_offset_y->setValue(v.shadow.offset.y()*100.0);
    d->ui.shadow_blur->setChecked(v.shadow.blur);
    d->ui.bbox->setChecked(v.bbox.enabled);
    d->ui.bbox_color->setColor(v.bbox.color);
    d->ui.bbox_hpadding->setValue(v.bbox.padding.x()*100.0);
    d->ui.bbox_vpadding->setValue(v.bbox.padding.y()*100.0);
    d->ui.spacing_line->setValue(v.spacing.line*100.0);
    d->ui.spacing_paragraph->setValue(v.spacing.paragraph*100.0);
}

auto OsdThemeWidget::value() const -> OsdTheme
{
    OsdTheme v;
    v.font.setFamily(d->ui.font_family->currentFont().family());
    d->ui.font_option->apply(v.font.qfont);
    v.font.color = d->ui.font_color->color();
//    v.font.scale = d->ui.font_scale->currentValue();
    v.font.size = d->ui.font_size->value()/100.0;
    v.outline.enabled = d->ui.outline->isChecked();
    v.outline.color = d->ui.outline_color->color();
    v.outline.width = d->ui.outline_width->value()/100.0;
    v.shadow.enabled = d->ui.shadow->isChecked();
    v.shadow.color = d->ui.shadow_color->color();
    v.shadow.offset.rx() = d->ui.shadow_offset_x->value()/100.0;
    v.shadow.offset.ry() = d->ui.shadow_offset_y->value()/100.0;
    v.shadow.blur = d->ui.shadow_blur->isChecked();
    v.bbox.enabled = d->ui.bbox->isChecked();
    v.bbox.color = d->ui.bbox_color->color();
    v.bbox.padding.rx() = d->ui.bbox_hpadding->value()/100.0;
    v.bbox.padding.ry() = d->ui.bbox_vpadding->value()/100.0;
    v.spacing.line = d->ui.spacing_line->value()/100.0;
    v.spacing.paragraph = d->ui.spacing_paragraph->value()/100.0;
    return v;
}

auto OsdThemeWidget::setTextAlphaChannel(bool on) -> void
{
    d->ui.font_color->setAlphaChannel(on);
    d->ui.outline_color->setAlphaChannel(on);
}

auto OsdThemeWidget::setShadowVsible(bool visible) -> void
{
    d->ui.shadow->setVisible(visible);
    d->ui.shadow_widget->setVisible(visible);
}

auto OsdThemeWidget::setOutlineVisible(bool visible) -> void
{
    d->ui.outline->setVisible(visible);
    d->ui.outline_widget->setVisible(visible);
}

auto OsdThemeWidget::setSpacingVisible(bool visible) -> void
{
    d->ui.spacing->setVisible(visible);
    d->ui.spacing_widget->setVisible(visible);
}

auto OsdThemeWidget::setBBoxVisible(bool visible) -> void
{
    d->ui.bbox->setVisible(visible);
    d->ui.bbox_widget->setVisible(visible);
}
