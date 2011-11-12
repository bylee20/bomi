#include "info.hpp"
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

Info Info::self;

Info::Info() {
	m_privPath = QString::fromLocal8Bit(qgetenv("CMPLAYER_PRIVATE_PATH"));
	if (m_privPath.isEmpty()) {
		QDir dir = QDir::home();
		if (!dir.exists(".cmplayer"))
			dir.mkdir(".cmplayer");
		dir.cd(".cmplayer");
		m_privPath = dir.absolutePath();
	}
	m_videoExt << "asf" << "avi" << "dvix" << "flv" << "mkv" << "mov"
			<< "mp4" << "mpeg" << "mpg" << "vob"
			<< "ogg" << "ogm"<< "qt" << "rm" << "rmvb" << "wmv";
	m_audioExt << "mp3" << "ogg" << "ra" << "wav" << "wma";
	m_subExt << "smi" << "srt" << "sub" << "txt";
	m_plExt << "pls" << "m3u";
}

Info::~Info() {}

QString Info::ExtList::toFilter() const {
	QString filter;
	for (QStringList::const_iterator it = begin(); it != end(); ++it)
		filter += "*." + *it + ' ';
	const int size = filter.size();
	if (size) {
		filter.remove(size-1, 1);
		return '(' + filter + ')';
	} else
		return QString();
}

QStringList Info::ExtList::toNameFilter() const {
	QStringList nameFilter;
	for (QStringList::const_iterator it = begin(); it != end(); ++it)
		nameFilter << ("*." + *it);
	return nameFilter;
}

QString Info::mediaExtFilter() {
	static const QString filter
		= QCoreApplication::translate("Info", "Video Files") + ' '
			+ Info::videoExt().toFilter() + ";;"
			+ QCoreApplication::translate("Info", "Audio Files") + ' '
			+ Info::audioExt().toFilter() + ";;"
			+ QCoreApplication::translate("Info", "All Files") + ' ' + "(*.*)";
	return filter;
}

const char *Info::pluginPath() {
	static QByteArray path;
	if (!path.isEmpty())
		return path.constData();
	path = qgetenv("CMPLAYER_VLC_PLUGIN_PATH");
	if (!path.isEmpty() && QDir(QString::fromLocal8Bit(path.data())).exists())
		return path.constData();
#ifdef CMPLAYER_VLC_PLUGIN_PATH
	path = CMPLAYER_VLC_PLUGIN_PATH;
	if (!path.isEmpty() && QDir(QString::fromLocal8Bit(path.data())).exists())
		return path.constData();
#endif
	path = QCoreApplication::applicationDirPath().toLocal8Bit();
	if (path.isEmpty())
		path = "./vlc-plugins";
	else
		path += "/vlc-plugins";
	return path.constData();
}

