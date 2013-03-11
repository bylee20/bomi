#include "info.hpp"

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
	m_videoExt << "3gp" << "3iv" << "asf" << "avi" << "cpk"
		<< "dat" << "divx" << "dv" << "flac" << "fli" << "flv"
		<< "h264" << "i263"	<< "m2ts" << "m4v" << "mkv" << "mov"
		<< "mp2" << "mp4" << "mpeg" << "mpg" << "mpg2" << "mpg4"
		<< "nsv" << "nut" << "nuv" << "ogg" <<"ogm" << "qt"
		<< "rm" << "rmvb" << "vcd" << "vfw" << "vob" << "wmv";
	m_audioExt << "aac" << "ac3" << "aiff" << "m4a" << "mka" << "mp3"
		<< "ogg" << "pcm" << "vaw" << "wav" << "waw" << "wma";
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
			+ QCoreApplication::translate("Info", "Image Files") + ' '
			+ Info::readableImageExt().toFilter() + ";;"
			+ QCoreApplication::translate("Info", "All Files") + ' ' + "(*.*)";
	return filter;
}

const char *Info::pluginPath() {
	return "";
}

static Info::ExtList convert(const QList<QByteArray> &formats) {
	Info::ExtList exts;
	for (auto &format : formats)
		exts << QString::fromLocal8Bit(format);
	return exts;
}

const Info::ExtList &Info::readableImageExt() {
	if (self.m_rImgExt.isEmpty())
		self.m_rImgExt = convert(QImageReader::supportedImageFormats());
	return self.m_rImgExt;
}

const Info::ExtList &Info::writableImageExt() {
	if (self.m_wImgExt.isEmpty())
		self.m_wImgExt = convert(QImageWriter::supportedImageFormats());
	return self.m_wImgExt;
}
