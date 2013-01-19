#ifndef RICHTEXTHELPER_HPP
#define RICHTEXTHELPER_HPP

#include "stdafx.hpp"
#include "richtextblock.hpp"

class RichTextHelper {
public:
	virtual ~RichTextHelper() {}

	static inline int toInt(const QStringRef &text) {
		int ret = 0;
		for (int i=0; i<text.size(); ++i) {
			const ushort c = text.at(i).unicode();
			if (Q_LIKELY('0' <= c && c <= '9'))
				ret = ret*10 + (c-'0');
			else
				break;
		}
		return ret;
	}

	static inline QColor toColor(const QStringRef &text) {
		if (!text.startsWith('#')) {
			int i=0;
			for (; i<text.size() && _IsHexNumber(text.at(i).unicode()); ++i) ;
			if (i == text.size())
				return QColor(_L("#") % text);
		}
		return QColor(text.toString());
	}

	static inline bool isRightBracket(ushort c) {return c == '>';}
	static inline bool isSeparator(ushort c) {return c == ' ' || c == '\t' || c == '\r' || c== '\n';}
	static inline bool isWhitespace(ushort c) {return c == ' ' || c == '\t';}
	static QString replace(const QStringRef &str, const QLatin1String &from, const QLatin1String &to, Qt::CaseSensitivity s = Qt::CaseInsensitive);
	static int indexOf(const QStringRef &ref, QRegExp &rx, int from = 0);
	static inline QStringRef trim(const QStringRef &text) {
		if (text.isEmpty()) return QStringRef();
		int start = 0, end = text.size();
		while (start < end && isSeparator(text.at(start).unicode())) ++start;
		while (end > start && isSeparator(text.at(end-1).unicode())) --end;
		return start < end ? _MidRef(text, start, end-start) : QStringRef();
	}

	static inline bool skipSeparator(int &pos, const QStringRef &text) {
		for (; pos < text.size() && isSeparator(text.at(pos).unicode()); ++pos) ;
		return pos >= text.size(); // true for end
	}
	static inline bool skipSeparator(int &pos, const QString &text) {return skipSeparator(pos, text.midRef(0));}

	static QChar entityCharacter(const QStringRef &entity);
	static int toFontPixelSize(const QStringRef &size);
	static int pixelSizeToPointSize(double pt);

	struct Tag {
		struct Attr {
			Attr() {}
			Attr(const QStringRef &name, const QStringRef &value): name(name), value(value) {}
			QStringRef name, value;
		};
		int index(const char *attr) const {for (int i=0; i<this->attr.size(); ++i) {if (_Same(this->attr[i].name, attr)) return i;}	return -1;}
		QStringRef value(const char *attr) const {const int idx = index(attr); return idx < 0 ? QStringRef() : this->attr[idx].value;}
		QMap<int, QVariant> style() const;
		int pos = -1;
		QStringRef name;
		QList<Attr> attr;
	};
	static Tag parseTag(const QStringRef &text, int &pos);
	static Tag parseTag(const QString &text, int &pos) {return parseTag(text.midRef(0, -1), pos);}
	static int innerText(const char *open, const char *close, const QStringRef &text, QStringRef &block, int &pos, Tag &tag);
	static int innerText(const char *open, const char *close, const QString &text, QStringRef &block, int &pos, Tag &tag) {
		return innerText(open, close, text.midRef(0, -1), block, pos, tag);
	}
};


class RichTextBlockParser : public RichTextHelper {
public:
	RichTextBlockParser(const QStringRef &text): m_text(text), m_pos(0), m_good(1) {}
	inline bool atEnd() const {return m_pos >= m_text.size();}
	QStringRef get(const char *open, const char *close, Tag *tag = nullptr);
	QList<RichTextBlock> paragraph(Tag *tag = nullptr);
protected:
	static QList<RichTextBlock> parse(const QStringRef &text, const RichTextBlock::Style &style = RichTextBlock::Style());
	QStringRef m_text;
	int m_pos, m_good;
};

#endif // RICHTEXTHELPER_HPP
