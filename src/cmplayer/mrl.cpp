#include "mrl.hpp"
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>

Mrl::Mrl(const QString &location) {
	const int idx = location.indexOf("://");
	if (idx < 0) {
		m_loc = QLatin1String("file://");
		m_loc += location;
	} else
		m_loc = location;
}

bool Mrl::isPlaylist() const {
	return fileName().endsWith(".pls", Qt::CaseInsensitive);
}

QString Mrl::fileName() const {
	const int idx = m_loc.lastIndexOf('/');
	return m_loc.mid(idx + 1);
}

QString Mrl::displayName() const {
	if (isLocalFile())
		return fileName();
	if (isDVD())
		return "DVD";
	return toString();
}

