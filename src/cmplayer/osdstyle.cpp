#include "osdstyle.hpp"
//#include "ui_osdstyle_widget.h"
#include <QtGui/QColorDialog>
#include <QtGui/QFontDialog>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include "record.hpp"

OsdStyle::OsdStyle()
	: color_bg(Qt::black), color_fg(Qt::white), shadow_color(Qt::black) {
	border_width = 0.05;
	text_scale = 0.03;
	auto_size = AutoSize::Width;
	has_shadow = false;
	shadow_offset = QPointF(0, 0);
	wrap_mode = QTextOption::WrapAtWordBoundaryOrAnywhere;
	shadow_blur = 3;
}

void OsdStyle::save(Record &r, const QString &group) const {
	r.beginGroup(group);
	r.write("font", font);
	r.write("color_bg", color_bg);
	r.write("color_fg", color_fg);
	r.write("border_width", border_width);
	r.write("text_scale", text_scale);
	r.write("auto_size", auto_size.name());
	r.write("has_shadow", has_shadow);
	r.write("shadow_color", shadow_color);
	r.write("shadow_offset", shadow_offset);
	r.write("shadow_blur", shadow_blur);
	r.endGroup();
}

void OsdStyle::load(Record &r, const QString &group) {
	r.beginGroup(group);
	font = r.read("font", font);
	color_bg = r.read("color_bg", color_bg);
	color_fg = r.read("color_fg", color_fg);
	border_width = r.read("border_width", border_width);
	text_scale = r.read("text_scale", text_scale);
	auto_size = r.readEnum("auto_size", AutoSize::Width);
	has_shadow = r.read("has_shadow", has_shadow);
	shadow_color = r.read("shadow_color", shadow_color);
	shadow_offset = r.read("shadow_offset", shadow_offset);
	shadow_blur = r.read("shadow_blur", shadow_blur);
	r.endGroup();
}

//struct OsdStyle::Widget::Data {
//	Ui::OsdStyle_Widget ui;
//	OsdStyle style;
//};

//OsdStyle::Widget::Widget(QWidget *parent)
//: QWidget(parent), d(new Data) {
//	d->ui.setupUi(this);
//	d->ui.fgColorLabel->setAutoFillBackground(true);
//	d->ui.bgColorLabel->setAutoFillBackground(true);
//	d->ui.scale->addItem(tr("Fit to Diagonal"), OsdStyle::FitToDiagonal);
//	d->ui.scale->addItem(tr("Fit to Height"), OsdStyle::FitToHeight);
//	d->ui.scale->addItem(tr("Fit to Width"), OsdStyle::FitToWidth);
//	setStyle(d->style);
//	connect(d->ui.fontButton, SIGNAL(clicked()), this, SLOT(slotFont()));
//	connect(d->ui.fgColorButton, SIGNAL(clicked()), this, SLOT(slotColor()));
//	connect(d->ui.bgColorButton, SIGNAL(clicked()), this, SLOT(slotColor()));
//}

//OsdStyle::Widget::~Widget() {
//	delete d;
//}

//void OsdStyle::Widget::slotColor() {
//	QLabel *label = 0;
//	QColor *before = 0;
//	if (sender() == d->ui.fgColorButton) {
//		label = d->ui.fgColorLabel;
//		before = &d->style.color_fg;
//	} else if (sender() == d->ui.bgColorButton) {
//		label = d->ui.bgColorLabel;
//		before = &d->style.color_bg;
//	} else
//		return;
//	bool ok = false;
//	const QRgb rgba = QColorDialog::getRgba(before->rgba(), &ok, this);
//	if (ok) {
//		before->setRgba(rgba);
//		setColor(label, *before);
//	}
//}

//void OsdStyle::Widget::slotFont() {
//	bool ok = false;
//	const QFont font = QFontDialog::getFont(&ok, d->style.font, this, tr("Select Font"), QFontDialog::DontUseNativeDialog);
//	if (ok)
//		updateFont(font);
//}

//void OsdStyle::Widget::updateFont(const QFont &font) {
//	d->style.font = font;
//	d->style.font.setPointSize(this->font().pointSize());
//	d->ui.fontLabel->setFont(d->style.font);
//	d->ui.fontLabel->setText(d->style.font.family());
//}

//void OsdStyle::Widget::setStyle(const OsdStyle &style) {
//	d->style = style;
//	updateFont(d->style.font);
//	setColor(d->ui.fgColorLabel, d->style.color_fg);
//	setColor(d->ui.bgColorLabel, d->style.color_bg);
//	d->ui.scale->setCurrentIndex(d->ui.scale->findData(d->style.auto_size));
//	d->ui.size->setValue(d->style.text_scale*100.);
//}

//void OsdStyle::Widget::setColor(QLabel *label, const QColor &color) {
//	QPalette p = label->palette();
//	p.setColor(QPalette::Window, color);
//	p.setColor(QPalette::Button, color);
//#define INVERT(value) (255 - value)
//	const QColor inverted(INVERT(color.red()), INVERT(color.green()), INVERT(color.blue()));
//#undef INVERT
//	p.setColor(QPalette::WindowText, inverted);
//	p.setColor(QPalette::ButtonText, inverted);
//	label->setPalette(p);
//	label->setText(color.name());
//}

//const OsdStyle &OsdStyle::Widget::style() const {
//	const int scale = d->ui.scale->currentIndex();
//	if (scale != -1)
//		d->style.auto_size = (OsdStyle::AutoSize)(d->ui.scale->itemData(scale).toInt());
//	d->style.text_scale = d->ui.size->value()/100.;
//	return d->style;
//}
