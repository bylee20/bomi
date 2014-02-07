#include "widgets.hpp"
#include "translator.hpp"
#include "playengine.hpp"
#include "qtcolorpicker.hpp"

EncodingComboBox::EncodingComboBox(QWidget *parent)
: QComboBox(parent) {
	enc = QStringList() << "UTF-8" << "Unicode"
			<< tr("Western European Languages") + " (CP1252)"
			<< tr("Western European Languages With Euro") + " (ISO-8859-15)"
			<< tr("Slavic/Central European Languages") + " (ISO-8859-2)"
			<< tr("Slavic/Central European Windows") + " (CP1250)"
			<< tr("Esperanto, Galician, Maltese, Turkish") + " (ISO-8859-3)"
			<< tr("Old Baltic Charset") + " (ISO-8859-4)"
			<< tr("Cyrillic") + " (ISO-8859-5)"
			<< tr("Cyrillic Windows") + " (CP1251)"
			<< tr("Arabic") + " (ISO-8859-6)"
			<< tr("Modern Greek") + " (ISO-8859-7)"
			<< tr("Turkish") + " (ISO-8859-9)"
			<< tr("Baltic") + " (ISO-8859-13)"
			<< tr("Celtic") + " (ISO-8859-14)"
			<< tr("Hebrew Charset") + " (ISO-8859-8)"
			<< tr("Russian") + " (KOI8-R)"
			<< tr("Ukrainian, Belarusian") + " (KOI8-U/RU)"
			<< tr("Simplified Chinese Charset") + " (CP936)"
			<< tr("Traditional Chinese Charset") + " (BIG5)"
			<< tr("Japanese Charset") + " (SHIFT-JIS)"
			<< tr("Korean Charset") + " (CP949)"
			<< tr("Thai Charset") + " (CP874)";
	addItems(enc);
	setEditable(true);
}

QString EncodingComboBox::encoding() const {
	QString enc = currentText().trimmed();
	QRegExp rxEnc(".* \\((.*)\\)");
	return (rxEnc.indexIn(enc) == -1) ? enc : rxEnc.cap(1);
}

void EncodingComboBox::setEncoding(const QString &encoding) {
	static const QRegExp rxEncoding(".* \\(" + encoding.toUpper() + "\\)");
	const int idx = enc.indexOf(rxEncoding);
	if (idx != -1)
		setCurrentIndex(idx);
	else
		setEditText(encoding);
}

/***************************************************************************************/

struct LocaleComboBox::Item {
	bool operator == (const Item &rhs) const { return locale == rhs.locale; }
	bool operator < (const Item &rhs) const { return name < rhs.name; }
	Item(const QLocale &locale): locale(locale) { }
	void update() { name = Translator::displayName(locale); }
	QString name;
	QLocale locale;
};

struct LocaleComboBox::Data {
	QList<Item> items;
};

LocaleComboBox::LocaleComboBox(QWidget *parent)
: QComboBox(parent), d(new Data) {
	auto locales = Translator::availableLocales();
	d->items.reserve(locales.size());
	for (auto &locale : locales)
		d->items.append(Item{locale});
	reset();
}

LocaleComboBox::~LocaleComboBox() {
	delete d;
}

void LocaleComboBox::reset() {
	auto locale = currentLocale();
	clear();
	for (auto &it : d->items)
		it.update();
	qSort(d->items);
	addItem(tr("System default locale"), QLocale::c());
	for (auto &it : d->items)
		addItem(it.name, it.locale);
	setCurrentIndex(findData(locale));
}


/***************************************************************************************/

struct FontOptionWidget::Data {
	QToolButton *bold, *italic, *underline, *strikeout;
	QToolButton *make_button(const QString &toolTip, const QIcon &icon, QWidget *parent) {
		QToolButton *button = new QToolButton(parent);
		button->setIcon(icon);
		button->setToolTip(toolTip);
		button->setCheckable(true);
		return button;
	}
};

FontOptionWidget::FontOptionWidget(QWidget *parent)
: QWidget(parent), d(new Data) {
	d->bold = d->make_button(tr("Bold"), QIcon(":/img/format-text-bold.png"), this);
	d->italic = d->make_button(tr("Italic"), QIcon(":/img/format-text-italic.png"), this);
	d->underline = d->make_button(tr("Underline"), QIcon(":/img/format-text-underline.png"), this);
	d->strikeout = d->make_button(tr("Strike Out"), QIcon(":/img/format-text-strikethrough.png"), this);
	QHBoxLayout *hbox = new QHBoxLayout(this);
	hbox->addWidget(d->bold);
	hbox->addWidget(d->italic);
	hbox->addWidget(d->underline);
	hbox->addWidget(d->strikeout);
	hbox->setMargin(0);
}

FontOptionWidget::~FontOptionWidget() {
	delete d;
}

void FontOptionWidget::set(bool bold, bool italic, bool underline, bool strikeout) {
	d->bold->setChecked(bold);
	d->italic->setChecked(italic);
	d->underline->setChecked(underline);
	d->strikeout->setChecked(strikeout);
}

bool FontOptionWidget::bold() const {
	return d->bold->isChecked();
}

bool FontOptionWidget::italic() const {
	return d->italic->isChecked();
}

bool FontOptionWidget::underline() const {
	return d->underline->isChecked();
}

bool FontOptionWidget::strikeOut() const {
	return d->strikeout->isChecked();
}

void FontOptionWidget::apply(QFont &font) {
	font.setBold(d->bold->isChecked());
	font.setItalic(d->italic->isChecked());
	font.setUnderline(d->underline->isChecked());
	font.setStrikeOut(d->strikeout->isChecked());
}

///////////////////////////////////////////////////////////////////////////////////////

struct ColorSelectWidget::Data {
	QtSolution::QtColorPicker *color;
	QLabel *alphaLabel;
	QDoubleSpinBox *alpha;
	bool hasAlpha;
};

ColorSelectWidget::ColorSelectWidget(QWidget *parent)
: QWidget(parent), d(new Data) {
	d->color = new QtSolution::QtColorPicker(this);
	d->color->setStandardColors();
	d->hasAlpha = false;
	d->alpha = new QDoubleSpinBox(this);
	d->alpha->setRange(0.0, 100.0);
	d->alpha->setSuffix("%");
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

}

ColorSelectWidget::~ColorSelectWidget() {
	delete d;
}

void ColorSelectWidget::setColor(const QColor &color, bool hasAlpha) {
	d->hasAlpha = hasAlpha;
	d->alpha->setVisible(d->hasAlpha);
	d->alpha->setValue(d->hasAlpha ? color.alphaF()*100.0 : 100.0);
	d->alphaLabel->setVisible(d->hasAlpha);
	d->color->setCurrentColor(color.rgb());
}

QColor ColorSelectWidget::color() const {
	QColor color = d->color->currentColor();
	color.setAlphaF(d->hasAlpha ? d->alpha->value()/100.0 : 1.0);
	return color;
}

