#include "subtitle_parser_p.hpp"
#include <QtCore/QTextCodec>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

int Subtitle::Parser::msPerChar = -1;

bool Subtitle::Parser::save(const Subtitle &sub, const QString &fileName) {
	QString content;
	if (!_save(content, sub))
		return false;
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Truncate))
		return false;
	QTextCodec *codec = QTextCodec::codecForName(m_enc.toLocal8Bit());
	if (!codec)
		return false;
	if (file.write(codec->fromUnicode(content)) < 0)
		return false;
	file.close();
	return true;
}

Subtitle Subtitle::Parser::parse(const QString &fileName) {
	Subtitle sub;
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly))
		return sub;
	QTextStream in;
	in.setDevice(&file);
	in.setCodec(m_enc.toLocal8Bit());
	m_all = in.readAll();
	m_name = fileName;
	m_pos = 0;
	_parse(sub);
	return sub;
}

QStringRef Subtitle::Parser::trimmed(const QStringRef &ref) {
	int from = 0;
	for (; from <ref.size() && RichString::isSeperator(ref.at(from).unicode()); ++from) ;
	int to = -1;
	for (int p=from; p<ref.size(); ++p) {
		if (RichString::isSeperator(ref.at(p).unicode())) {
			if (to < 0)
				to = p;
		} else
			to = -1;
	}
	if (to < 0)
		return RichString::midRef(ref, from);
	return RichString::midRef(ref, from, to - from);
}

QStringRef Subtitle::Parser::processLine(int &idx, const QString &contents) {
	int from = idx;
	idx = contents.indexOf(QLatin1Char('\n'), from);
	if (idx < 0)
		idx = contents.indexOf(QLatin1Char('\r'), from);
	if (idx < 0) {
		idx = contents.size();
		return contents.midRef(from);
	} else {
		return contents.midRef(from, (idx++) - from);
	}
}

int Subtitle::Parser::predictEndTime(const Component::const_iterator &it) {
	if (msPerChar > 0)
		return it.value().text.size()*msPerChar + it.key();
	return -1;
}

QString &Subtitle::Parser::replaceEntity(QString &str) {
	return str.replace("<", "&lt;").replace(">", "&gt;").replace(" ", "&nbsp;");
}

Subtitle::Parser *Subtitle::Parser::create(const QString &fileName) {
	QFileInfo info(fileName);
	const QString ext = info.suffix();
#define EXT_IS(_ext) (ext.compare(QLatin1String(_ext), Qt::CaseInsensitive) == 0)
	if (EXT_IS("smi"))
		return new Sami;
	else if(EXT_IS("srt"))
		return new SubRip;
	else if (EXT_IS("sub") || EXT_IS("txt")) {
		QFile file(info.absoluteFilePath());
		if (!file.open(QFile::ReadOnly))
			return 0;
		QTextStream in(&file);
		for (int i=0; i<10 && !in.atEnd(); ++i) {
			const QString line = in.readLine();
			if (MicroDVD::rxLine.indexIn(line) != -1)
				return new MicroDVD;
			else if (TMPlayer::rxLine.indexIn(line) != -1)
				return new TMPlayer;
		}
	}
#undef EXT_IS
	return 0;
}

