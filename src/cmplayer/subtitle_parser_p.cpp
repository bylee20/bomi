#include "subtitle_parser_p.hpp"
#include "tagiterator.hpp"
#include <QtCore/QRegExp>
#include "global.hpp"
#include <QtCore/QDebug>
#include <QtCore/QLinkedList>
#include <QtGui/QTextFormat>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtCore/QStringBuilder>

bool SamiParser::isParsable() const {
	if (same(file().suffix(), "smi") || same(file().suffix(), "sami"))
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
		if (same(tag.name, "body"))
			break;
		if (same(tag.name, "sync")) {
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
			SubtitleComponent *comp = nullptr;
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
	if (same(file().suffix(), "srt"))
		return true;
	return false;
}

void SubRipParser::_parse(Subtitle &sub) {
	QRegExp rxCaption(
		"(^|[\r\n]*)(\\d+)(\r|\r\n|\n)"
		"(\\d\\d):(\\d\\d):(\\d\\d),(\\d\\d\\d) --> (\\d\\d):(\\d\\d):(\\d\\d),(\\d\\d\\d)"
		"(.*)([\r\n]*$|\r\r|\r\n\r\n|\n\n)"
	);
	rxCaption.setMinimal(true);

	QRegExp rxLineBreak("(\r[^\n]|\r\n|[^\r]\n)");
	sub.clear();
	auto &comp = append(sub);
	int pos = 0;
	auto toInt = [&rxCaption] (int nth) {return rxCaption.cap(nth).toInt();};
	QLinkedList<SubtitleComponent> caps;
	while (pos < all().size()) {
		const int idx = rxCaption.indexIn(all(), pos);
		if (idx < 0)
			break;
		caps.append(SubtitleComponent(file().fileName()));
		auto &part = caps.last();
		const int start = timeToMSec(toInt(4), toInt(5), toInt(6), toInt(7));
		const int end = timeToMSec(toInt(8), toInt(9), toInt(10), toInt(11));
		const QString text = _L("<p>") % rxCaption.cap(12).trimmed().replace(rxLineBreak, "<br>") % _L("</p>");
		append(part, text, start, end);
		comp.unite(part, 25);
		pos = idx + rxCaption.matchedLength();
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
		const int time = timeToMSec(toInt(1), toInt(2), toInt(3));
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
		append(sub, SubtitleComponent::Time);
	} else {
		seekTo(0);
		append(sub, SubtitleComponent::Frame);
	}

	QRegExp rxAttr("\\{([^\\}]+):([^\\}]+)\\}");
	SubtitleComponent &comp = components(sub).first();
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
			if (same(name, "y")) {
				if (value.contains(_L('i'), Qt::CaseInsensitive))
					addTag0("i");
				if (value.contains(_L('u'), Qt::CaseInsensitive))
					addTag0("u");
				if (value.contains(_L('s'), Qt::CaseInsensitive))
					addTag0("s");
				if (value.contains(_L('b'), Qt::CaseInsensitive))
					addTag0("b");
			} else if (same(name, "c")) {
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
