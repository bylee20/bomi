#include "subtitle_parser_p.hpp"
#include "global.hpp"

bool SamiParser::isParsable() const {
	if (_Same(file().suffix(), "smi") || _Same(file().suffix(), "sami"))
		return true;
	if (skipSeparators())
		return false;
	if (all().startsWith("<sami", Qt::CaseInsensitive))
		return true;
	return false;
}

void SamiParser::_parse(Subtitle &sub) {
	const QString &text = this->all();
	sub.clear();
	int pos = 0;
	while (pos < text.size()) {
		const QChar c = text.at(pos);
		if (c.unicode() != '<') {
			++pos;
			continue;
		}
		Tag tag = parseTag(text, pos);
		if (_Same(tag.name, "body"))
			break;
		if (_Same(tag.name, "sync")) {
			pos = tag.pos;
			break;
		}
	}
	RichTextBlockParser parser(text.midRef(pos));
	auto &comps = components(sub);
	while (!parser.atEnd()) {
		Tag tag;
		const QStringRef block_sync = parser.get("sync", "/?sync|/body|/sami", &tag);
		if (tag.name.isEmpty())
			break;
		const int sync = toInt(tag.value("start"));
		QMap<QString, QList<RichTextBlock> > blocks;
		RichTextBlockParser p(block_sync);
		while (!p.atEnd()) {
			const QList<RichTextBlock> paragraph = p.paragraph(&tag);
			blocks[tag.value("class").toString()] += paragraph;
		}
		for (auto it = blocks.begin(); it != blocks.end(); ++it) {
			SubComp *comp = nullptr;
			for (int i=0; i<sub.count(); ++i) {
				if (comps[i].language() == it.key()) {
					comp = &comps[i];
					break;
				}
			}
			if (!comp) {
				comp = &append(sub);
				comp->m_klass = it.key();
			}
			(*comp)[sync] += it.value();
		}
	}

//	for (SubtitleComponent &comp : comps) {
//		if (comp.size() < 3)
//	}
}



bool SubRipParser::isParsable() const {
	if (_Same(file().suffix(), "srt"))
		return true;
	return false;
}

void SubRipParser::_parse(Subtitle &sub) {
	QRegularExpression rxNum(R"(^\s*(\d+)\s*$)");
	QRegularExpression rxTime(R"(^\s*(\d\d):(\d\d):(\d\d),(\d\d\d)\s*-->\s*(\d\d):(\d\d):(\d\d),(\d\d\d)\s*$)");
	QRegularExpression rxBlank(R"(^\s*$)");
	auto getNumber = [&rxNum, this] () {
		for (;;) {
			const auto ref = getLine();
			if (ref.isNull())
				break;
			auto matched = rxNum.match(ref.toString());
			if (matched.hasMatch())
				return matched.capturedRef(1).toInt();
		}
		return -1;
	};
	auto getTime = [&rxTime, this] (int &start, int &end) {
		for (;;) {
			const auto ref = getLine();
			if (ref.isNull())
				break;
			auto matched = rxTime.match(ref.toString());
			if (matched.hasMatch()) {
#define TO_INT(n) (matched.capturedRef(n).toInt())
				start = _TimeToMSec(TO_INT(1), TO_INT(2), TO_INT(3), TO_INT(4));
				end = _TimeToMSec(TO_INT(5), TO_INT(6), TO_INT(7), TO_INT(8));
#undef TO_INT
				return true;
			}
		}
		return false;
	};
	auto getCaption = [&rxBlank, this] () {
		QString ret;
		for (;;) {
			const auto line = getLine().toString();
			auto matched = rxBlank.match(line);
			if (matched.hasMatch())
				break;
			if (!ret.isEmpty())
				ret += "<br>";
			ret += line;
		}
		return QString("<p>" % ret % "</p>");
	};

	sub.clear();
	auto &comp = append(sub);
	QLinkedList<SubComp> caps;
	for (;;) {
		const auto num = getNumber();
		if (num < 0)
			break;
		int start = 0, end = 0;
		if (!getTime(start, end))
			break;
		const auto caption = getCaption();
		append(comp, caption, start, end);
	}
}

void  TMPlayerParser::_parse(Subtitle &sub) {
	sub.clear();
	auto &comp = append(sub);
	int predictedEnd = -1;
	auto toInt = [this] (int nth) {return rxLine.cap(nth).toInt();};
	while (!atEnd()) {
		auto line = getLine().toString();
		if (line.indexOf(rxLine) == -1)
			continue;
		const int time = _TimeToMSec(toInt(1), toInt(2), toInt(3));
		if (predictedEnd > 0 && time > predictedEnd)
			comp[predictedEnd];
		QString text = rxLine.cap(4);
		predictedEnd = predictEndTime(time, text);
		text = _L("<p>") % encodeEntity(trim(text.midRef(0))) % _L("</p>");
		append(comp, text, time);
	}
}

void MicroDVDParser::_parse(Subtitle &sub) {
	QString line;
	int begin = -1;
	while (!atEnd() && begin == -1) {
		line = trim(getLine()).toString();
		begin = rxLine.indexIn(line);
	}
	if (begin == -1)
		return;
	bool ok = false;
	const double fps = rxLine.cap(3).toDouble(&ok);
	auto getKey = [ok, fps] (int frame) {return ok ? qRound((frame/fps)*1000.0) : frame;};
	if (ok) {
		append(sub, SubComp::Time);
	} else {
		seekTo(0);
		append(sub, SubComp::Frame);
	}

	QRegExp rxAttr("\\{([^\\}]+):([^\\}]+)\\}");
	SubComp &comp = components(sub).first();
	while (!atEnd()) {
		line = trim(getLine()).toString();
		if (rxLine.indexIn(line) == -1)
			continue;
		const int start = getKey(rxLine.cap(1).toInt());
		const int end = getKey(rxLine.cap(2).toInt());
		QString text = rxLine.cap(3);
		QString parsed1, parsed2;
		auto addTag0 = [&parsed1, &parsed2, this] (const QString &name) {
			parsed1 += _L('<') % name % _L('>');
			parsed2 += _L("</") % name % _L('>');
		};
		auto addTag1 = [&parsed1, &parsed2, this] (const QString &name, const QString &attr) {
			parsed1 += _L('<') % name % _L(' ') % attr % _L('>');
			parsed2 += _L("</") % name % _L('>');
		};
		int idx = 0;
		QRegExp rxColor("\\$([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");
		while (text.indexOf(rxAttr, idx) != -1) {
			const auto name = rxAttr.cap(1);
			const auto value = rxAttr.cap(2);
			if (_Same(name, "y")) {
				if (value.contains(_L('i'), Qt::CaseInsensitive))
					addTag0("i");
				if (value.contains(_L('u'), Qt::CaseInsensitive))
					addTag0("u");
				if (value.contains(_L('s'), Qt::CaseInsensitive))
					addTag0("s");
				if (value.contains(_L('b'), Qt::CaseInsensitive))
					addTag0("b");
			} else if (_Same(name, "c")) {
				if (rxColor.indexIn(value) != -1)
					addTag1(_L("font"), _L("color=\"#") % rxColor.cap(3) % rxColor.cap(2) % rxColor.cap(1) %_L("\""));
			}
			idx = rxAttr.pos() + rxAttr.matchedLength();
		}
		if (idx < text.size()) {
			if (text[idx] == _L('/')) {
				addTag0("i");
				++idx;
			}
			text = _L("<p>") % parsed1 % replace(text.midRef(idx), _L("|"), _L("<br>")) % parsed2 % _L("</p>");
		} else
			text = _L("<p>") % parsed1 % parsed2 % _L("</p>");
		append(comp, text, start, end);
	}
}
