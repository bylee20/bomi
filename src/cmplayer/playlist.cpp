#include "playlist.hpp"
#include "downloader.hpp"
#include "info.hpp"
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTextStream>
#include <QtCore/QRegExp>
#include <QtCore/QTextCodec>
#include <QtCore/QDir>

Playlist::Playlist()
: QList<Mrl>() {}

Playlist::Playlist(const Playlist &rhs)
: QList<Mrl>(rhs) {}

Playlist::Playlist(const Mrl &mrl): QList<Mrl>() {
	push_back(mrl);
}

Playlist::Playlist(const QList<Mrl> &rhs)
: QList<Mrl>(rhs) {}

bool Playlist::save(const QString &filePath, Type type) const {
	QFile file(filePath);
	if (!file.open(QFile::WriteOnly | QFile::Truncate))
		return false;
	if (type == Unknown)
		type = getType(file.fileName());
	switch (type) {
	case PLS:
		return savePLS(&file);
	case M3U:
		return saveM3U(&file);
	default:
		return false;
	}
}

Playlist &Playlist::loadAll(const QDir &dir) {
	clear();
	const QStringList filter = Info::mediaNameFilter();
	const QStringList files = dir.entryList(filter, QDir::Files, QDir::Name);
	for (int i=0; i<files.size(); ++i)
		push_back(dir.absoluteFilePath(files[i]));
	return *this;
}

bool Playlist::load(const QString &filePath, const QString &enc, Type type) {
	QFile file(filePath);
	return load(&file, enc, type);
}

bool Playlist::load(QFile *file, const QString &enc, Type type) {
	clear();
	if (!file->isOpen() && !file->open(QFile::ReadOnly))
		return false;
	if (type == Unknown)
		type = getType(file->fileName());
	switch (type) {
	case PLS:
		return loadPLS(file, enc);
	case M3U:
		return loadM3U(file, enc);
	default:
		return false;
	}
}

bool Playlist::load(const Mrl &mrl, const QString &enc, Type type) {
	if (mrl.isLocalFile())
		return load(mrl.toLocalFile(), enc, type);
	QTemporaryFile file(QDir::tempPath() + "/cmplayer_temp_XXXXXX_" + mrl.fileName());
	if (!file.open() || !Downloader::get(mrl.toString(), &file, 30000))
		return false;
	return load(&file, enc, type);
}

Playlist::Type Playlist::getType(const QString &fileName) {
	const QString suffix = QFileInfo(fileName).suffix().toLower();
	if (suffix == "pls")
		return PLS;
	else if (suffix == "m3u")
		return M3U;
	else
		return Unknown;
}

bool Playlist::savePLS(QFile *file) const {
	QTextStream out(file);
	const int count = size();
	out << "[playlist]" << endl << "NumberOfEntries=" << count << endl << endl;
	for (int i=0; i<count; ++i)
		out << "File" << i+1 << '=' << at(i).toString() << endl
				<< "Length" << i+1 << '=' << -1 << endl << endl;
	out << "Version=2" << endl;
	return true;
}

bool Playlist::saveM3U(QFile *file) const {
	QTextStream out(file);
	const int count = size();
	out << "#EXTM3U\n";
	for (int i=0; i<count; ++i) {
		out << "#EXTINF:" << 0 << ',' << "" << '\n';
		out << at(i).toString() << '\n';
	}
	return true;
}


bool Playlist::loadPLS(QFile *file, const QString &enc) {
	QTextStream in(file);
	if (!enc.isEmpty())
		in.setCodec(QTextCodec::codecForName(enc.toLocal8Bit()));
	const qint64 pos = in.pos();
	in.seek(0);
	while (!in.atEnd()) {
		const QString line = in.readLine();
		if (line.isEmpty())
			continue;
		static QRegExp rxFile("^File\\d+=(.+)$");
		if (rxFile.indexIn(line) != -1)
			append(Mrl(rxFile.cap(1)));
	}
	in.seek(pos);
	return true;
}

bool Playlist::loadM3U(QFile *file, const QString &enc) {
	QTextStream in(file);
	if (!enc.isEmpty())
		in.setCodec(QTextCodec::codecForName(enc.toLocal8Bit()));
	const qint64 pos = in.pos();
	in.seek(0);
	while (!in.atEnd()) {
		const QString line = in.readLine().trimmed();
		if (!line.isEmpty() && !line.startsWith("#"))
			append(Mrl(line));
	}
	in.seek(pos);
	return true;
}

void Playlist::save(const QString &name, QSettings *set) const {
	set->beginWriteArray(name, size());
	for (int i=0; i<size(); ++i) {
		set->setArrayIndex(i);
		set->setValue("mrl", at(i).toString());
	}
	set->endArray();
}

void Playlist::load(const QString &name, QSettings *set) {
	clear();
	const int size = set->beginReadArray(name);
	for (int i=0; i<size; ++i) {
		set->setArrayIndex(i);
		push_back(set->value("mrl", QString()).toString());
	}
	set->endArray();
}
