#include "richtexthelper.hpp"
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtCore/QLinkedList>
#include <QtCore/QStringBuilder>
#include <QtCore/QDebug>

QString RichTextHelper::replace(const QStringRef &str, const QLatin1String &from, const QLatin1String &to, Qt::CaseSensitivity s) {
	QString text;
	int start = 0;
	const int len = strlen(from.latin1());
	for (;;) {
		const int pos = str.indexOf(from, start, s);
		if (pos < 0) {
			text += midRef(str, start);
			break;
		} else {
			text += midRef(str, start, pos - start);
			text += to;
			start = pos + len;
		}
	}
	return text;
}

int RichTextHelper::pixelSizeToPointSize(double pt) {
	const double dpi = QApplication::desktop()->logicalDpiY();
	return pt*dpi/72.0 + 0.5;
}

int RichTextHelper::toFontPixelSize(const QStringRef &size) {
	int px = 0, i = 0;
	while (i<size.size()) {
		const ushort c = size.at(i).unicode();
		if ('0' <= c && c <= '9') {
			px = px*10 + (c - '0');
		} else
			break;
		++i;
	}
	if (i >= size.size())
		return px;
	const QStringRef unit = midRef(size, i);
	if (unit.size() == 2 && unit.compare("pt", Qt::CaseInsensitive) == 0)
		px = pixelSizeToPointSize(px);
	return px;
}

RichTextHelper::Tag RichTextHelper::parseTag(const QStringRef &text, int &pos) {
	auto at = [&text] (int idx) {return text.at(idx).unicode();};
	if (at(pos) != '<')
		return Tag();
	Tag tag;
	tag.pos = pos;
	++pos;
	if (skipSeparator(pos, text))
		return Tag();
	if (at(pos) == '!') { // skip comment
		pos = text.indexOf(QLatin1String("!--"), pos);
		if (pos < 0) {
			pos = text.size();
			return Tag();
		}
		tag.name = midRef(text, pos, 3);
		pos = text.indexOf('>', pos);
		if (pos < 0)
			pos = text.size();
		return tag;
	}
	int start = pos;
	while (pos < text.size() && !isSeparator(at(pos)) && at(pos) != '>')
		++pos;
	tag.name = midRef(text, start, pos - start);
	if (tag.name.startsWith('/')) {
		while (at(pos) != '>' && pos < text.size())
			++pos;
		if (pos >= text.size())
			return Tag();
		++pos;
		return tag;
	}
	if (skipSeparator(pos, text))
		return Tag();
	ushort q = 0;
	while (pos < text.size()) {
		if (skipSeparator(pos, text))
			return Tag();
		const ushort c = at(pos);
		if (c == '>') {
			++pos;
			return tag;
		}
		start = pos;
		while (pos < text.size()) {
			const ushort c = at(pos);
			if (isSeparator(c) || c == '=')
				break;
			++pos;
		}
		Tag::Attr attr;
		attr.name = midRef(text, start, pos - start);
		if (skipSeparator(pos, text))
			return Tag();
		if (at(pos) == '=') {
			// find value
			if (skipSeparator(++pos, text))
				return Tag();
			if (at(pos) == '\'') {
				q = '\'';
				++pos;
			} else if (at(pos) ==  '"') {
				q = '"';
				++pos;
			} else
				q = 0;
			start = pos;
			while (pos < text.size()) {
				const ushort c = at(pos);
				const bool q_end = (q && c == q && at(pos-1) != '\\');
				if (q_end || (!q && (isSeparator(c) || c == '>'))) {
					if (q_end)
						++pos;
					attr.value = midRef(text, start, pos - start);
					tag.attr.push_back(attr);
					break;
				}
				++pos;
			}
		} else {
			tag.attr.push_back(attr);
		}
	}
	return Tag();
}

QChar RichTextHelper::entityCharacter(const QStringRef &entity) {
	Q_UNUSED(entity);
#define RETURN(ent, c) {if (same(entity, ent)) return QChar(c);}
	RETURN("nbsp", ' ');
	RETURN("amp", '&');
	RETURN("lt", '<');
	RETURN("gt", '>');
#undef RETURN
	return QChar();
}

int RichTextHelper::indexOf(const QStringRef &ref, QRegExp &rx, int from) {
	const int pos = ref.position();
	const int idx = ref.string()->indexOf(rx, from + pos) - pos;
	return 0 <= idx && idx < ref.length() ? idx : -1;
}


int RichTextHelper::innerText(const char *open, const char *close, const QStringRef &text, QStringRef &block, int &pos, Tag &tag) {
	while (pos < text.size()) {
		const QChar c = text.at(pos);
		if (c.unicode() == '<') {
			tag = parseTag(text, pos);
			if (same(tag.name, open))
				break;
		} else
			++pos;
	}
	if (pos >= text.size() || tag.name.isEmpty())
		return 0;
	int ret = 1;
	typedef QLatin1String _L;
	QRegExp rx(_L("<[\\s\\n\\r]*(") % _L(close) % _L(")(>|[^0-9a-zA-Z>]+[^>]*>)"));
	rx.setCaseSensitivity(Qt::CaseInsensitive);
	int start = pos;
	int end = indexOf(text, rx, start);
	if (end < 0) {
		end = pos = text.size();
	} else {
		pos = end;
		const QString cap = rx.cap(1);
		if (Q_UNLIKELY(cap.startsWith('/'))) {
			const QStringRef closer = cap.midRef(1, -1);
			if (same(closer, open))
				pos += rx.matchedLength();
			if (same(closer, "body") || same(closer, "sami"))
				ret = -1;
		}
	}
	block = midRef(text, start, end - start);
	return ret;
}

RichTextBlock::Style RichTextHelper::Tag::style() const {
	RichTextBlock::Style style;
	if (same(name, "b"))
		style[QTextFormat::FontWeight] = QFont::Bold;
	else if (same(name, "u"))
		style[QTextFormat::FontUnderline] = true;
	else if (same(name, "i"))
		style[QTextFormat::FontItalic] = true;
	else if (same(name, "s") || same(name, "strike"))
		style[QTextFormat::FontStrikeOut] = true;
	else if (same(name, "sup"))
		style[QTextFormat::TextVerticalAlignment] = QTextCharFormat::AlignSuperScript;
	else if (same(name, "sub"))
		style[QTextFormat::TextVerticalAlignment] = QTextCharFormat::AlignSubScript;
	else if (same(name, "font")) {
		for (int i=0; i<attr.size(); ++i) {
			if (same(attr[i].name, "color")) {
				style[QTextFormat::ForegroundBrush] = QBrush(toColor(trim(attr[i].value)));
			} else if (same(attr[i].name, "face"))
				style[QTextFormat::FontFamily] = trim(attr[i].value).toString();
			else if (same(attr[i].name, "size"))
				style[QTextFormat::FontPixelSize] = toFontPixelSize(trim(attr[i].value));
		}
	}
	return style;
}

QStringRef RichTextBlockParser::get(const char *open, const char *close, Tag *tag) {
	Tag _tag;
	if (!tag)
		tag = &_tag;
	QStringRef ret;
	m_good = innerText(open, close, m_text, ret, m_pos, *tag);
	if (m_good <= 0)
		m_pos = m_text.size();
	return ret;
}

QList<RichTextBlock> RichTextBlockParser::paragraph(Tag *tag) {
	if (m_pos >= m_text.size())
		return QList<RichTextBlock>();
	QStringRef paragraph = trim(get("p", "/?sync|/?p|/body|/sami", tag));
	if (m_good)
		return parse(paragraph, tag ? tag->style() : RichTextBlock::Style());
	return QList<RichTextBlock>();
}

QList<RichTextBlock> RichTextBlockParser::parse(const QStringRef &text, const RichTextBlock::Style &style) {
	QList<RichTextBlock> ret;

	auto add_format = [&ret] (const RichTextBlock::Style &style) {
		ret.last().formats << RichTextBlock::Format();
		ret.last().formats.last().begin = ret.last().text.size();
		ret.last().formats.last().end = -1;
		ret.last().formats.last().style = style;
	};

	auto add_block = [&ret, &add_format] (bool paragraph, const RichTextBlock::Style &style) {
		ret << RichTextBlock(paragraph);
		add_format(style);
	};

	QLinkedList<RichTextBlock::Style> fmtStack;
	QLinkedList<QStringRef> tagStack;
	add_block(true, style);

	int pos = 0;

	auto add_text = [&ret] (const QStringRef &text) {
		int pos = 0;
		while (pos < text.size()) {
			const QChar c = text.at(pos);
			if (isSeparator(c.unicode())) {
				if (!ret.last().text.isEmpty())
					ret.last().text.append(c);
				if (skipSeparator(pos, text))
					break;
			} else {
				if (c.unicode() == '&') {
					const int idx = text.indexOf(';', ++pos);
					if (idx < 0)
						ret.last().text.append(c);
					else {
						ret.last().text.append(entityCharacter(midRef(text, pos, idx - pos)));
						pos = idx + 1;
					}
				} else {
					ret.last().text.append(c);
					++pos;
				}
			}
		}
	};

	while (pos <text.size()) {
		const int idx = text.indexOf('<', pos);
		if (idx < 0) {
			add_text(midRef(text, pos, -1));
			break;
		} else {
			add_text(midRef(text, pos, idx - pos));
			pos = idx;
		}
		Tag tag = parseTag(midRef(text, 0, -1), pos);
		if (tag.name.isEmpty())
			continue;
		ret.last().formats.last().end = ret.last().text.size();

		if (same(tag.name, "br")) { // new block
			add_block(false, ret.last().formats.last().style);
		} else {
			if (tag.name.startsWith('/')) { // restore format
				auto fmtIt = fmtStack.begin();
				auto tagIt = tagStack.begin();
				for (; tagIt != tagStack.end(); ++tagIt, ++fmtIt) {
					if (tagIt->compare(midRef(tag.name, 1)) == 0) {
						add_format(*fmtIt);
						fmtStack.erase(fmtIt);
						tagStack.erase(tagIt);
						break;
					}
				}
			} else { // new format
				if (same(tag.name, "ruby")) {
					QRegExp rx("(<\\s*rb\\s*>)?([^<]*)(<\\s*/rb\\s*>)?<\\s*rt\\s*>([^<]*)(<\\s*/rt\\s*>)?(<\\s*/ruby\\s*>|$)");
					const int idx = indexOf(text, rx, pos);
					if (idx < 0)
						continue;
					add_format(ret.last().formats.last().style);
					ret.last().rubies.push_back(RichTextBlock::Ruby());
					RichTextBlock::Ruby &ruby = ret.last().rubies.last();
					RichTextBlock::Format &rb_format = ret.last().formats.last();
					rb_format.mergeStyle(tag.style());

					ruby.rb_begin = rb_format.begin = ret.last().text.size();
					add_text(rx.cap(2).midRef(0, -1));
					ruby.rb_end = rb_format.end = ret.last().text.size();
					ruby.rb_style = rb_format.style;
					ruby.rt = rx.cap(4);

					add_format(ret.last().formats[ret.last().formats.size()-2].style);

					pos = idx + rx.matchedLength();
				} else {
					fmtStack.push_front(ret.last().formats.last().style);
					tagStack.push_front(tag.name);
					add_format(ret.last().formats.last().style);
					ret.last().formats.last().mergeStyle(tag.style());
				}
			}
		}
	}

	if (ret.last().formats.last().end < 0)
		ret.last().formats.last().end = ret.last().text.size();

	return ret;
}
