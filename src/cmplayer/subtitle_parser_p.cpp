#include "subtitle_parser_p.hpp"
#include "tagiterator.hpp"
#include "global.hpp"
#include <QtCore/QDebug>
#include <QtCore/QLinkedList>

void Subtitle::Parser::Sami::_parse(Subtitle &sub) {
	int sync = -1;
	TagIterator tag(all());
	for (;;) {
		const int pos = tag.next();
		if (pos < 0)
			break;
		if (tag.elementIs("sync")) {
			sync = pos;
			break;
		}
	}
	if (sync < 0)
		return;

	QMap<QString, int> idxes;
	const QString header = all().left(sync);
	for (int i = 0;;) {
		static QRegExp rxLang("\\s*\\.(\\w+)\\s*\\{(.+)\\}");
		i = rxLang.indexIn(header, i);
		if (i < 0)
			break;
		static QRegExp rxParam("\\s*(\\S+)\\s*:\\s*(\\S+)\\s*;\\s*");
		int pos = 0;
		Component comp(name());
		comp.m_lang.m_klass = rxLang.cap(1);
		const QString text = rxLang.cap(2);
		while ((pos = rxParam.indexIn(text, pos)) != -1) {
			const QString key = rxParam.cap(1);
			if (!key.compare("name", Qt::CaseInsensitive))
				comp.m_lang.m_name = rxParam.cap(2);
			else if (!key.compare("lang", Qt::CaseInsensitive))
				comp.m_lang.m_locale = rxParam.cap(2);
			pos += rxParam.matchedLength();
		}
		const int idx = idxes.size();
		idxes.insert(comp.m_lang.m_klass, idx);
		sub.m_comp.push_back(comp);
		i += rxLang.matchedLength();
	}

	for (;;) {
		if (sync < 0)
			break;
		int time = tag.valueToInt("start");
		QString klass = tag.value("class").toString();
		QString text, plain;
		int brIdx = -1;
		QLinkedList<QStringRef> pair;
		for (;;) {
			int a = tag.pos();
			const bool br = tag.elementIs("br");
			if (br) {
				if (!text.isEmpty()) {
					if (brIdx == -1)
						brIdx = text.size();
					appendTo(text, tag);
					plain += '\n';
				}
			} else {
				brIdx = -1;
				appendTo(text, tag);
				if (tag.elementIs("p") || tag.elementIs("sync") || tag.elementIs("span") || tag.elementIs("font"))
					pair.push_front(tag.element());
				else if (!tag.element().isEmpty() && tag.element().at(0) == '/') {
					for (auto it=pair.begin(); it != pair.end(); ++it) {
						if (it->compare(RichString::midRef(tag.element(), 1), Qt::CaseInsensitive) == 0) {
							pair.erase(it);
							break;
						}
					}
				}
			}
			const int idx = tag.next();
			const int count = idx < 0 ? -1 : idx - a;
			const QStringRef ref = all().midRef(a, count);
			const int prev = text.size();
			RichString::process(ref, text, plain, false);
			if (brIdx != -1) {
				for (int i=text.size()-1; i>=prev; --i) {
					if (!isspace(text[i].unicode())) {
						brIdx = -1;
						break;
					}
				}
			}
			if (idx < 0) {
				sync = -1;
				break;
			}
			if (klass.isEmpty())
				klass = tag.value("class").toString();
			if (tag.elementIs("sync")) {
				sync = idx;
				break;
			} else if (tag.elementIs("/body")) {
				sync = -1;
				break;
			}
		}

		int chop = 0;
		for (int i=plain.size()-1; i>=0 && isspace(plain[i].unicode()); --i, ++chop) ;
		plain.chop(chop);
		if (brIdx != -1)
			text.chop(text.size() - brIdx);
		for (const QStringRef &one : const_(pair)) {
			text += QLatin1String("</");
			text += one;
			text += QLatin1Char('>');
		}

		Node node;
		node.text = RichString(text, plain);
		QMap<QString, int>::const_iterator it = idxes.find(klass);
		if (it == idxes.end()) {
			const int idx = idxes.size();
			idxes.insert(klass, idx);
			Component comp(name());
			comp.m_lang.m_klass = klass;
			sub.m_comp.push_back(comp);
		}
		if (!node.text.hasWords())
			node.text.clear();
		sub.m_comp[idxes[klass]].insert(time, node);
	}
	for (int i=0; i<sub.m_comp.size(); ++i) {
		Component &comp = sub.m_comp[i];
		if (comp.isEmpty())
			continue;
		const int end = predictEndTime(comp.end()-1);
		if (end >= 0)
			comp[end] = Node();
	}
}

bool Subtitle::Parser::Sami::needToAddSharp(const QStringRef &name, const QStringRef &value) {
	if (value.size() != 6)
		return false;
	if (!TagIterator::opEq(name, "color"))
		return false;
	for (int i=0; i<6; ++i) {
		const ushort ucs = value.at(i).unicode();
		if (!RichString::isHexNumber(ucs))
			return false;
	}
	return true;
}

QString &Subtitle::Parser::Sami::appendTo(QString &rich, const TagIterator &tag) {
	if (tag.element().isEmpty())
		return rich;
	rich += '<';
	if (tag.elementIs("p"))
		rich += QLatin1String("span");
	else
		rich += tag.element();
	for (int i=0; i<tag.attributeCount(); ++i) {
		const TagIterator::Attr &attr = tag.attribute(i);
		rich += ' ';
		rich += attr.name;
		if (!attr.value.isEmpty()) {
			rich += QLatin1Char('=');
			if (attr.q.unicode() != '\0')
				rich += attr.q;
			if (needToAddSharp(attr.name, attr.value))
				rich += QLatin1Char('#');
			rich += attr.value;
			if (attr.q.unicode() != '\0')
				rich += attr.q;
		}
	}
	if (!tag.isOpen())
		rich += '>';
	return rich;
}

QRegExp Subtitle::Parser::TMPlayer::rxLine("^\\s*(\\d?\\d)\\s*:\\s*(\\d\\d)\\s*:\\s*(\\d\\d)\\s*:\\s*(.*)$");

void  Subtitle::Parser::TMPlayer::_parse(Subtitle &sub) {
	sub.append(Component(name()));
	Component &comp = sub.m_comp[0];
	int predictedEnd = -1;
	while (!atEnd()) {
		const QString line = getLine();
		if (rxLine.indexIn(line) == -1)
			continue;
#define T_INT(nth) rxLine.cap(nth).toInt()
		const int time = timeToMSec(T_INT(1), T_INT(2), T_INT(3));
#undef T_INT
		Subtitle::Node node;
		if (predictedEnd > 0 && time > predictedEnd)
			comp.insert(predictedEnd, node);
		QString text = rxLine.cap(4);
		replaceEntity(text).replace('|', "<br>");
		node.text = RichString(text);
		predictedEnd = predictEndTime(comp.insert(time, node));
	}
}

QRegExp Subtitle::Parser::MicroDVD::rxLine("^\\{(\\d+)\\}\\{(\\d+)\\}(.*)$");

void Subtitle::Parser::MicroDVD::_parse(Subtitle &sub) {
	sub.append(Component(name(), Component::Frame));
	Component &comp = sub.m_comp[0];
	while (!atEnd()) {
		const QString line = getLine();
		if (rxLine.indexIn(line) == -1)
			continue;
		const int start = rxLine.cap(1).toInt();
		const int end = rxLine.cap(2).toInt();
		QString text = rxLine.cap(3).trimmed();
		text.replace('|', "<br>");
		Node node;
		node.text = RichString(text);
		comp.insert(start, node);
		comp.insert(end, Node());
	}
}

void Subtitle::Parser::SubRip::_parse(Subtitle &sub) {
	sub.append(Component(name()));
	Component &comp = sub.m_comp[0];
	QString line;
	while (!atEnd()) {
		line = getLine();
		static QRegExp rxNum("^(\\d+)$");
#define RX_TIME "(\\d\\d):(\\d\\d):(\\d\\d),(\\d\\d\\d)"
		static QRegExp rxTime("^"RX_TIME" --> "RX_TIME"$");
#undef RX_TIME
		if (rxNum.indexIn(line) == -1)
			continue;
		while (!atEnd()) {
			line = getLine();
			if (rxTime.indexIn(line) != -1)
				break;
		}
		if (atEnd())
			return;
#define T_INT(nth) rxTime.cap(nth).toInt()
		const int start = timeToMSec(T_INT(1), T_INT(2), T_INT(3), T_INT(4));
		const int end = timeToMSec(T_INT(5), T_INT(6), T_INT(7), T_INT(8));
#undef T_INT
		QString text;
		while (!atEnd()) {
			line = getLine().trimmed();
			if (line.isEmpty())
				break;
			text += line + '\n';
		}
		text.chop(1);
		text.replace('\n', "<br>");
		Node node;
		node.text = text;
		comp.insert(start, node);
		comp.insert(end, Node());
	}
}
