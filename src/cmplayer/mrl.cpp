#include "mrl.hpp"
#include "info.hpp"

Mrl::Mrl(const QString &location) {
	const int idx = location.indexOf("://");
	if (idx < 0) {
		m_loc = _L("file://");
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

QString Mrl::suffix() const {
	const int idx = m_loc.lastIndexOf('.');
	if (idx != -1)
		return m_loc.mid(idx + 1);
	return QString();
}

QString Mrl::displayName() const {
	if (isLocalFile())
		return fileName();
	if (isDvd())
		return "DVD";
	return toString();
}

bool Mrl::isImage() const { return Info::readableImageExt().contains(suffix(), Qt::CaseInsensitive); }
