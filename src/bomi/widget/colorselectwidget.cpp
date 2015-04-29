#include "colorselectwidget.hpp"
#include "qtcolorpicker.hpp"

struct ColorSelectWidget::Data {
    QtSolution::QtColorPicker *color;
    QLabel *alphaLabel;
    QDoubleSpinBox *alpha;
    bool hasAlpha = false;
};

ColorSelectWidget::ColorSelectWidget(QWidget *parent)
: QWidget(parent), d(new Data) {
    d->color = new QtSolution::QtColorPicker(this);
    d->color->setStandardColors();
    d->hasAlpha = false;
    d->alpha = new QDoubleSpinBox(this);
    d->alpha->setRange(0.0, 100.0);
    d->alpha->setSuffix(qApp->translate("PrefDialog", " %"));
    d->alpha->setDecimals(1);
    d->alpha->setAccelerated(true);
    d->alphaLabel = new QLabel(tr("Transparency"));
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->addWidget(d->color);
    hbox->addWidget(d->alphaLabel);
    hbox->addWidget(d->alpha);
    hbox->setMargin(0);

    d->alpha->hide();
    d->alphaLabel->hide();

    connect(d->alpha, SIGNAL(valueChanged(double)), this, SIGNAL(colorChanged()));
    connect(d->color, SIGNAL(colorChanged(QColor)), this, SIGNAL(colorChanged()));
}

ColorSelectWidget::~ColorSelectWidget() {
    delete d;
}

auto ColorSelectWidget::setColor(const QColor &color) -> void
{
    d->alpha->setVisible(d->hasAlpha);
    d->alpha->setValue(d->hasAlpha ? color.alphaF()*100.0 : 100.0);
    d->alphaLabel->setVisible(d->hasAlpha);
    d->color->setCurrentColor(color.rgb());
}

auto ColorSelectWidget::color() const -> QColor
{
    QColor color = d->color->currentColor();
    color.setAlphaF(d->hasAlpha ? d->alpha->value()/100.0 : 1.0);
    return color;
}

auto ColorSelectWidget::setAlphaChannel(bool on) -> void
{
    d->hasAlpha = on;
}

auto ColorSelectWidget::hasAlphaChannel() const -> bool
{
    return d->hasAlpha;
}
