//#include "richtext.hpp"
//#include <QtGui/QPainter>
//#include <QtCore/QDebug>
//#include <QtGui/QApplication>
//#include <QtGui/QDesktopWidget>
//#include <QtCore/QVector>

//struct RichTextRuby {
//	RichTextRuby() {
//		layout.setTextOption(option);
//		overwritten = 0;
//	}
//	void setLayout(const QString &text, const QTextLayout::FormatRange &format);
//	QTextLayout layout;
//	int overwritten;
//	int range_begin, range_end;
//private:
//	static const QTextOption option;
//	static QTextOption makeOption() {
//		QTextOption opt;
//		opt.setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
//		opt.setWrapMode(QTextOption::NoWrap);
//		return opt;
//	}
//};

//const QTextOption RichTextRuby::option = RichTextRuby::makeOption();

//void RichTextRuby::setLayout(const QString &text, const QTextLayout::FormatRange &format) {
//	layout.setText(text);
//	QTextLayout::FormatRange fmt = format;
//	fmt.format.setProperty(QTextFormat::FontPixelSize, (int)((double)fmt.format.intProperty(QTextFormat::FontPixelSize)*0.45));
//	overwritten = fmt.format.intProperty(RichTextParser::Overwritten);
//	layout.setAdditionalFormats(QList<QTextLayout::FormatRange>() << fmt);
//}


//static int pixelSizeToPointSize(double pt) {
//	const double dpi = QApplication::desktop()->logicalDpiY();
//	return pt*dpi/72.0 + 0.5;
//}

//static int toFontPixelSize(const QStringRef &size) {
//	int px = 0, i = 0;
//	while (i<size.size()) {
//		const ushort c = size.at(i).unicode();
//		if ('0' <= c && c <= '9') {
//			px = px*10 + (c - '0');
//		} else
//			break;
//		++i;
//	}
//	if (i >= size.size())
//		return px;
//	const QStringRef unit = RichTextParser::midRef(size, i);
//	if (unit.size() == 2 && unit.compare("pt", Qt::CaseInsensitive) == 0)
//		px = pixelSizeToPointSize(px);
//	return px;
//}

////static QColor toColor(const QStringRef &ref) {
////	QList<int> rgbs;

////}


//RichTextParser::~RichTextParser() {
//	clear();
//}

//void RichTextParser::clear() {
//	qDeleteAll(m_lines);
//	m_text.clear();
//	m_ref.clear();
//	m_tag.clear();
//	m_lines.clear();
//	m_trace.clear();
//}

//void RichTextParser::copyFrom(const RichTextParser &other) {
//	clear();
//	m_format = other.m_format;
//	m_option = other.m_option;
//	m_size = other.m_size;
//	m_natural = other.m_natural;
//	m_lines.resize(other.m_lines.size());
//	for (int i=0; i<m_lines.size(); ++i) {
//		RichTextLines *lines = new RichTextLines;
//		RichTextLines *from = other.m_lines[i];
//		lines->setLayout(from->layout.text(), from->layout.additionalFormats());
//		lines->ruby.resize(from->ruby.size());
//		for (int j=0; j<lines->ruby.size(); ++j) {
//			RichTextRuby *ruby = new RichTextRuby;
//			ruby->layout.setText(from->ruby[j]->layout.text());
//			ruby->layout.setAdditionalFormats(from->ruby[j]->layout.additionalFormats());
//			ruby->overwritten = from->ruby[j]->overwritten;
//			ruby->range_begin = from->ruby[j]->range_begin;
//			ruby->range_end = from->ruby[j]->range_end;
//			lines->ruby[j] = ruby;
//		}
//		m_lines[i] = lines;
//	}

//}

//QStringRef RichTextParser::trim(const QStringRef &text) {
//	int start = 0, end = text.size();
//	while (isSeperator(text.at(start).unicode()) && start < end)
//		++start;
//	while (isSeperator(text.at(end-1).unicode()) && end > start)
//		--end;
//	return start < end ? midRef(text, start, end-start) : QStringRef();
//}

//int RichTextParser::setFormat(const QStringRef &tag, const QStringRef &attr, const QStringRef &value, Format &format) {
//	int over = 0;
//	if (same(tag, "font")) {
//		if (same(attr, "color")) {
//			format.setProperty(Format::ForegroundBrush, QBrush(QColor(trim(value).toString())));
//			over |= ForegroundColor;
//		} else if (same(attr, "face")) {
//			format.setProperty(Format::FontFamily, trim(value).toString());
//			over |= FontFamily;
//		} else if (same(attr, "size")) {
//			format.setProperty(Format::FontPixelSize, toFontPixelSize(trim(value)));
//			over |= FontPixelSize;
//		}
//	} else if (same(tag, "ruby")) {
//	}
//	if (same(attr, "style")) {

//	}
//	return over;
//}

//int RichTextParser::setFormat(const QStringRef &tag, QTextCharFormat &format) {
//	int over = 0;
//	if (same(tag, "b")) {
//		format.setProperty(QTextCharFormat::FontWeight, QFont::Bold);
//		over |= FontWeight;
//	} else if (same(tag, "u")) {
//		format.setProperty(QTextCharFormat::FontUnderline, true);
//		over |= FontUnderline;
//	} else if (same(tag, "i")) {
//		format.setProperty(QTextFormat::FontItalic, true);
//		over |= FontItalic;
//	} else if (same(tag, "sup")) {
//		format.setProperty(QTextFormat::TextVerticalAlignment, QTextCharFormat::AlignSuperScript);
//	} else if (same(tag, "sub"))
//		format.setProperty(QTextFormat::TextVerticalAlignment, QTextCharFormat::AlignSubScript);
//	return over;
//}

//QStringRef RichTextParser::parseTag(int &pos, QTextCharFormat &format, int &overwritten) {
//	++pos;
//	if (skipSeperator(pos, m_ref))
//		return QStringRef();
//	int start = pos;
//	while (!isSeperator(at(pos)) && at(pos) != '>')
//		++pos;
//	const QStringRef tag = midRef(m_ref, start, pos - start);
//	if (tag.startsWith('/')) {
//		const int idx = m_tag.lastIndexOf(midRef(tag, 1));
//		if (idx >= 0) {
//			format = m_trace[idx];
//			overwritten = format.intProperty(Overwritten);
//		}
//		++pos;
//		return tag;
//	}
//	overwritten |= setFormat(tag, format);
//	if (skipSeperator(pos, m_ref))
//		return tag;
//	ushort q = 0;
//	while (pos < m_ref.size()) {
//		if (skipSeperator(pos, m_ref))
//			return QStringRef();
//		const ushort c = at(pos);
//		if (c == '>') {
//			++pos;
//			return tag;
//		}
//		start = pos;
//		while (pos < m_ref.size()) {
//			const ushort c = at(pos);
//			if (isSeperator(c) || c == '=')
//				break;
//			++pos;
//		}
//		const QStringRef attr = midRef(m_ref, start, pos - start);
//		if (skipSeperator(pos, m_ref))
//			return QStringRef();
//		if (at(pos) == '=') {
//			// find value
//			if (skipSeperator(++pos, m_ref))
//				return QStringRef();
//			if (at(pos) == '\'') {
//				q = '\'';
//				++pos;
//			} else if (at(pos) ==  '"') {
//				q = '"';
//				++pos;
//			} else
//				q = 0;
//			start = pos;
//			while (pos < m_ref.size()) {
//				const ushort c = at(pos);
//				if ((q && c == q && at(pos-1) != '\\') || (!q && (isSeperator(c) || c == '>'))) {
//					overwritten |= setFormat(tag, attr, midRef(m_ref, start, pos - start), format);
//					break;
//				}
//				++pos;
//			}
//		} else {
//			// no value attributes
//			overwritten |= setFormat(tag, attr, QStringRef(), format);
//		}
//	}
//	return QStringRef();
//}

//void RichTextParser::applyDefaultFormat() {
//	int over = 0;
//	auto restore = [this, &over] (Format &format, Style s, Property p) {
//		if (!(over & s))
//			format.setProperty(p, m_format.property(p));
//	};
//	for (auto &lines : m_lines) {
//		auto formats = lines->layout.additionalFormats();
//		for (auto &format : formats) {
//			over = format.format.intProperty(Overwritten);
//			restore(format.format, ForegroundColor, Format::ForegroundBrush);
//			restore(format.format, FontPixelSize, Format::FontPixelSize);
//			restore(format.format, FontWeight, Format::FontWeight);
//			restore(format.format, FontFamily, Format::FontFamily);
//			restore(format.format, FontItalic, Format::FontItalic);
//			restore(format.format, FontUnderline, Format::FontUnderline);
//			restore(format.format, FontStrikeOut, Format::FontStrikeOut);
//		}
//		lines->layout.setAdditionalFormats(formats);
//		for (auto &ruby : lines->ruby) {
//			over = ruby->overwritten;
//			auto formats = ruby->layout.additionalFormats();
//			Format &format = formats.first().format;
//			restore(format, ForegroundColor, Format::ForegroundBrush);
//			if (!(over & FontPixelSize))
//				format.setProperty(Format::FontPixelSize, (int)(m_format.intProperty(Format::FontPixelSize)*0.45));
//			restore(format, FontWeight, Format::FontWeight);
//			restore(format, FontFamily, Format::FontFamily);
//			restore(format, FontItalic, Format::FontItalic);
//			restore(format, FontUnderline, Format::FontUnderline);
//			restore(format, FontStrikeOut, Format::FontStrikeOut);
//			ruby->layout.setAdditionalFormats(formats);
//		}
//	}
//}

//bool RichTextParser::setText(const QString &text) {
//	clear();
//	m_text = text;
//	m_ref = text.midRef(0);
//	QTextLayout::FormatRange range, rt_fmt;
//	int overwritten = 0;
//	int pos = 0, start = 0;
//	range.start = 0;	range.format = m_format;
//	RichTextLines *lines = new RichTextLines;

//	RichTextRuby *ruby = nullptr;
//	QList<QTextLayout::FormatRange> ranges;
//	QString part;
//	int rt_start = -1, rt_end = -1;

//	auto push_back_format = [this, &ruby, &part, &overwritten, &ranges, &range, &pos, &start, &text, &rt_start] () {
//		range.start = part.size();
//		range.length = pos - start;
//		range.format.setProperty(Overwritten, overwritten);
//		part.append(text.midRef(start, range.length));
//		if (ruby)
//			ruby->range_end += range.length;
//		ranges.push_back(range);
//		m_trace.push_back(range.format);
//	};

//	auto push_back_lines = [this, &part, &ranges, &lines, &overwritten] () {
//		lines->setLayout(part, ranges);
//		part.clear();
//		ranges.clear();
//		m_lines.push_back(lines);
//	};

//	while (pos < text.size()) {
//		const QChar c = text.at(pos);
//		if (c.unicode() == '<') {
//			if (start >= 0)
//				push_back_format();
//			const QStringRef tag = parseTag(pos, range.format, overwritten);
//			if (tag.isNull())
//				return false;
//			if (same(tag, "br")) {
//				push_back_lines();
//				lines = new RichTextLines;
//			} else if (same(tag, "ruby")) {
//				if (!ruby)
//					ruby = new RichTextRuby;
//				ruby->range_end = ruby->range_begin = part.size();
//			} else if (same(tag, "rt")) {
//				if (!ruby)
//					return false;
//				m_tag.pop_back();
//				rt_start = pos;
//				QRegExp rxRt("<\\s*/[rR][tT]\\s*>");
//				rt_end = text.indexOf(rxRt, rt_start);
//				if (rt_end < 0) {
//					QRegExp rxRuby("<\\s*/[rR][uU][bB][yY]\\s*>");
//					rt_end = text.indexOf(rxRuby, rt_start);
//					if (rt_end < 0)
//						return false;
//					pos = rt_end;
//				} else
//					pos = rt_end + rxRt.matchedLength();

//				rt_fmt.format = range.format;
//				rt_fmt.start = 0;
//				rt_fmt.length = rt_end - rt_start;
//				ruby->setLayout(text.mid(rt_start, rt_fmt.length), rt_fmt);
//			} else if (same(tag, "/ruby") && ruby) {
////				ruby->range_end = pos;
//				if (!ruby->layout.text().isEmpty())
//					lines->ruby.push_back(ruby);
//				else
//					delete ruby;
//				ruby = nullptr;
//			}
//			m_tag.push_back(tag);
//			start = pos;
//		} else
//			++pos;
//	}
//	if (start < text.size())
//		push_back_format();
//	if (!part.isEmpty())
//		push_back_lines();
//	return true;
//}

//static const double MaxLeading = 1e10;

//void RichTextParser::updateLayout(double maxWidth) {
//	updateLayout(maxWidth, MaxLeading*10.0);
//}

//void RichTextParser::updateLayout(double maxWidth, double leading) {
//	if (m_lines.isEmpty())
//		return;
//	QPointF pos(0, 0);
//	double width = 0;
//	for (int i=0; i<m_lines.size(); ++i) {
//		m_lines[i]->layout.setTextOption(m_option);
//		pos.rx() = 0;
//		m_lines[i]->layout.beginLayout();
//		int rt_idx = 0;
//		for (;;) {
//			QTextLine line = m_lines[i]->layout.createLine();
//			if (!line.isValid())
//				break;
//			line.setLineWidth(maxWidth);
//			line.setPosition(pos);
//			double rt_height = 0.0;
//			while (rt_idx < m_lines[i]->ruby.size()) {
//				RichTextRuby &ruby = *m_lines[i]->ruby[rt_idx];
//				if (!(line.textStart() <= ruby.range_begin && ruby.range_end <= line.textStart() + line.textLength()))
//					break;
//				const int left = line.cursorToX(ruby.range_begin);
//				const int right = line.cursorToX(ruby.range_end);
//				if (left < right) {
//					ruby.layout.beginLayout();
//					QTextLine line_rt = ruby.layout.createLine();
//					if (line_rt.isValid()) {
//						line_rt.setLineWidth(right - left);
//						line_rt.setPosition(QPointF(left, pos.y()));
//						if (rt_height < line_rt.height())
//							rt_height = line_rt.height();
//					}
//					ruby.layout.endLayout();
//				}
//				++rt_idx;
//			}
//			if (leading > MaxLeading)
//				leading = line.leading();
//			pos.ry() += qMax(rt_height, leading);
//			line.setPosition(pos);
//			pos.ry() += line.height();
//			if (line.naturalTextWidth() > width)
//				width = line.naturalTextWidth();
//		}
//		m_lines[i]->layout.endLayout();
//	}
//	m_size = QSizeF(maxWidth, pos.y());
//	m_natural = QSizeF(width, pos.y());
//}

//void RichTextParser::draw(QPainter *painter, const QPointF &pos) {
//	painter->save();
//	for (auto &lines : m_lines) {
//		lines->layout.draw(painter, pos);
//		for (auto &ruby : lines->ruby) {
//			ruby->layout.draw(painter, pos);
//		}
//	}
//	painter->restore();
//}
