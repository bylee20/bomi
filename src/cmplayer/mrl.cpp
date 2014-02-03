#include "mrl.hpp"
#include "info.hpp"

Mrl::Mrl(const QUrl &url) {
	if (url.isLocalFile())
		m_loc = _L("file://") % url.toLocalFile();
	else
		m_loc = url.toString();
}

Mrl::Mrl(const QString &location, const QString &name) {
	const int idx = location.indexOf("://");
	if (idx < 0)
		m_loc = _L("file://") % QFileInfo(location).absoluteFilePath();
	else if (location.startsWith("file://", Qt::CaseInsensitive))
		m_loc = QUrl::fromPercentEncoding(location.toUtf8());
	else if (location.startsWith("dvd", Qt::CaseInsensitive))
		m_loc = location;
	else
		m_loc = QUrl::fromPercentEncoding(location.toUtf8());
	m_name = name;
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
	if (!m_name.isEmpty())
		return m_name;
	if (isLocalFile())
		return fileName();
	if (isDvd())
		return "DVD";
	return location();
}

bool Mrl::isImage() const { return Info::readableImageExt().contains(suffix(), Qt::CaseInsensitive); }

bool Mrl::isEmpty() const {
	const int idx = m_loc.indexOf("://");
	return (idx < 0) || !(idx+3 < m_loc.size());
}
