#include "fontoptionwidget.hpp"
#include <QToolButton>

struct FontOptionWidget::Data {
    FontOptionWidget *p = nullptr;
    QToolButton *bold, *italic, *underline, *strikeout;
    auto make(const QString &toolTip, const char *icon) -> QToolButton*
    {
        QToolButton *button = new QToolButton(p);
        button->setIcon(QIcon(_L(icon)));
        button->setToolTip(toolTip);
        button->setCheckable(true);
        return button;
    }
};

FontOptionWidget::FontOptionWidget(QWidget *parent)
: QWidget(parent), d(new Data) {
    d->p = this;
    d->bold = d->make(tr("Bold"), ":/img/format-text-bold.png");
    d->italic = d->make(tr("Italic"), ":/img/format-text-italic.png");
    d->underline = d->make(tr("Underline"), ":/img/format-text-underline.png");
    d->strikeout = d->make(tr("Strike Out"),
                           ":/img/format-text-strikethrough.png");
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->addWidget(d->bold);
    hbox->addWidget(d->italic);
    hbox->addWidget(d->underline);
    hbox->addWidget(d->strikeout);
    hbox->setMargin(0);

    auto signal = &FontOptionWidget::changed;
    PLUG_CHANGED(d->bold);
    PLUG_CHANGED(d->italic);
    PLUG_CHANGED(d->underline);
    PLUG_CHANGED(d->strikeout);
}

FontOptionWidget::~FontOptionWidget() {
    delete d;
}

auto FontOptionWidget::set(bool bold, bool italic, bool underline, bool strikeout) -> void
{
    d->bold->setChecked(bold);
    d->italic->setChecked(italic);
    d->underline->setChecked(underline);
    d->strikeout->setChecked(strikeout);
}

auto FontOptionWidget::bold() const -> bool
{
    return d->bold->isChecked();
}

auto FontOptionWidget::italic() const -> bool
{
    return d->italic->isChecked();
}

auto FontOptionWidget::underline() const -> bool
{
    return d->underline->isChecked();
}

auto FontOptionWidget::strikeOut() const -> bool
{
    return d->strikeout->isChecked();
}

auto FontOptionWidget::apply(QFont &font) -> void
{
    font.setBold(d->bold->isChecked());
    font.setItalic(d->italic->isChecked());
    font.setUnderline(d->underline->isChecked());
    font.setStrikeOut(d->strikeout->isChecked());
}
