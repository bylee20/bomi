#include "globalqmlobject.hpp"

bool GlobalQmlObject::m_dbc = false;

GlobalQmlObject::GlobalQmlObject(QObject *parent)
: QObject(parent) {
	m_font = qApp->font();
}

QString GlobalQmlObject::monospace() const {
#ifdef Q_OS_MAC
	return "monaco";
#else
	return "monospace";
#endif
}

double GlobalQmlObject::textWidth(const QString &text, int size, const QString &family) const {
	QFont font(family);
	font.setPixelSize(size);
	QFontMetricsF metrics(font);
	return metrics.width(text);
}

double GlobalQmlObject::textWidth(const QString &text, int size) const {
	return textWidth(text, size, m_font.family());
}

