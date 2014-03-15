#include "mrl.hpp"
#include "info.hpp"

Mrl::Mrl(const QUrl &url) {
	if (url.isLocalFile())
		m_loc = _L("file://") % url.toLocalFile();
	else
		m_loc = url.toString();
}

Mrl::Mrl(const QString &location, const QString &name) {
	if (location.isEmpty())
		return;
	const int idx = location.indexOf("://");
	if (idx < 0)
		m_loc = _L("file://") % QFileInfo(location).absoluteFilePath();
	else if (location.startsWith("file://", Qt::CaseInsensitive))
		m_loc = QUrl::fromPercentEncoding(location.toUtf8());
	else if (location.startsWith("dvdnav://", Qt::CaseInsensitive) || location.startsWith("bd://", Qt::CaseInsensitive))
		m_loc = location;
	else
		m_loc = QUrl::fromPercentEncoding(location.toUtf8());
	m_name = name;
}

bool Mrl::isPlaylist() const {
	return Info::playlistExt().contains(suffix(), Qt::CaseInsensitive);
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
		return _L("DVD");
	if (isBluray())
		return _L("Blu-ray");
	return location();
}

bool Mrl::isImage() const { return Info::readableImageExt().contains(suffix(), Qt::CaseInsensitive); }

bool Mrl::isEmpty() const {
	const int idx = m_loc.indexOf("://");
	return (idx < 0) || !(idx+3 < m_loc.size());
}

static const QStringList discSchemes = QStringList() << _L("dvdnav") << _L("bd");

bool Mrl::isDisc() const {
	return discSchemes.contains(scheme(), Qt::CaseInsensitive);
}

QString Mrl::device() const {
	const auto scheme = this->scheme();
	if (!discSchemes.contains(scheme, Qt::CaseInsensitive))
		return QString();
	auto path = m_loc.midRef(scheme.size() + 3);
	const int idx = path.indexOf(_L('/'));
	if (idx < 0)
		return QString();
	return path.mid(idx+1).toString();
}

Mrl Mrl::fromDisc(const QString &scheme, const QString &device, int title) {
	QString loc = scheme % _L("://");
	if (title == 0)
		loc += _L("menu");
	else if (title > 0)
		loc += QString::number(title);
	return Mrl(loc % _L('/') % device);
}
