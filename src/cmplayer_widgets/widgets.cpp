#include "widgets.hpp"
#include <QtCore/QDebug>
#include <QtPlugin>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>
#include <QtCore/QEvent>

Q_EXPORT_PLUGIN2(cmplayer_widgets, Widgets)

Label::Label(QWidget *parent)
: QFrame(parent), m_elide(Qt::ElideNone), m_alignment(Qt::AlignLeft | Qt::AlignVCenter) {
}

QSize Label::minimumSizeHint() const {
	if (m_elide == Qt::ElideNone)
		return sizeHint();
	const QFontMetrics& fm = fontMetrics();
	return QSize(fm.width("..."), fm.height());
}

QSize Label::sizeHint() const {
	const QFontMetrics& fm = fontMetrics();
	return QSize(fm.width(m_text), fm.height());
}

void Label::paintEvent(QPaintEvent *event) {
	QFrame::paintEvent(event);
	const QFontMetrics &fm = fontMetrics();
	const QRect rect = contentsRect();
	const QString text = fm.width(m_text) > rect.width() ? fm.elidedText(m_text, m_elide, rect.width()) : m_text;
	QPainter painter(this);
	painter.drawText(contentsRect(), m_alignment, text);
}

void Label::changeEvent(QEvent *event) {
	QFrame::changeEvent(event);
	if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange)
		updateAll();
}

LabelPlugin::LabelPlugin(QObject *parent)
: QObject(parent), WidgetPlugin("Label",
"<ui language=\"c++\">\n"
"	<widget class=\"Label\" name=\"label\">\n"
"		<property name=\"sizePolicy\">\n"
"			<sizepolicy hsizetype=\"Expanding\" vsizetype=\"Fixed\">\n"
"				<horstretch>0</horstretch>\n"
"				<verstretch>0</verstretch>\n"
"			</sizepolicy>\n"
"		</property>\n"
"	</widget>\n"
"	<customwidgets>\n"
"		<customwidget>\n"
"			<class>Label</class>\n"
"			<propertyspecifications>\n"
"				<stringpropertyspecification name=\"text\" notr=\"true\" type=\"singleline\" />\n"
"			</propertyspecifications>\n"
"		</customwidget>\n"
"	</customwidgets>\n"
"</ui>\n"
) {
}

QWidget *LabelPlugin::createWidget(QWidget *parent) {
	return new Label(parent);
}

Widgets::Widgets(QObject *parent): QObject(parent) {
	m_widgets << new LabelPlugin(this);
}
