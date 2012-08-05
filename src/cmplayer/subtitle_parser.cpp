#include "subtitle_parser_p.hpp"
#include <QtCore/QTextCodec>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

int SubtitleParser::msPerChar = -1;

bool SubtitleParser::save(const Subtitle &sub, const QString &fileName, const QString &enc) {
	QString content;
	if (!_save(content, sub))
		return false;
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Truncate))
		return false;
	QTextCodec *codec = QTextCodec::codecForName(enc.toLocal8Bit());
	if (!codec)
		return false;
	if (file.write(codec->fromUnicode(content)) < 0)
		return false;
	file.close();
	return true;
}

Subtitle SubtitleParser::parse(const QString &fileName, const QString &enc) {
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly) || file.size() > (1 << 20))
		return Subtitle();
	QTextStream in;
	in.setDevice(&file);
	in.setCodec(enc.toLocal8Bit());
	const QString all = in.readAll();
	QFileInfo info(fileName);
	Subtitle sub;

	auto tryIt = [&sub, &all, &info] (SubtitleParser *p) {
		p->m_all = all;
		p->m_file = info;
		const bool parsable = p->isParsable();
		if (parsable)
			p->_parse(sub);
		delete p;
		return parsable;
	};

	if (tryIt(new SamiParser) || tryIt(new SubRipParser) || tryIt(new MicroDVDParser) || tryIt(new TMPlayerParser))
		return sub;
	return Subtitle();
}

QStringRef SubtitleParser::processLine(int &idx, const QString &contents) {
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

int SubtitleParser::predictEndTime(int start, const QString &text) {
	if (msPerChar > 0)
		return start + text.size()*msPerChar;
	return -1;
}

int SubtitleParser::predictEndTime(const SubtitleComponent::const_iterator &it) {
	if (msPerChar > 0)
		return it.value().totalLength()*msPerChar + it.key();
	return -1;
}

QString SubtitleParser::encodeEntity(const QStringRef &str) {
	QString ret;
	for (int i=0; i<str.size(); ++i) {
		ushort c = str.at(i).unicode();
		switch (c) {
		case ' ':
			ret += _L("&nbsp;");
			break;
		case '<':
			ret += _L("&lt;");
			break;
		case '>':
			ret += _L("&gt;");
			break;
		case '|':
			ret += _L("<br>");
			break;
		default:
			ret += str.at(i);
			break;
		}
	}
	return ret;
}

QStringRef SubtitleParser::getLine() const {
	int from = m_pos;
	int end = -1;
	while (m_pos < m_all.size()) {
		if (at(m_pos) == '\r') {
			end = m_pos;
			++m_pos;
			if (m_pos < m_all.size() && at(m_pos) == '\n')
				++m_pos;
			break;
		} else if (at(m_pos) == '\n') {
			end = m_pos;
			++m_pos;
			break;
		}
		++m_pos;
	}
	if (end < 0)
		end = m_all.size();
	return from < m_all.size() ? m_all.midRef(from, end - from) : QStringRef();
}

