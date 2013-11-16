#ifndef RICHTEXTBLOCK_HPP
#define RICHTEXTBLOCK_HPP

#include "stdafx.hpp"
#include "richtexthelper.hpp"

struct RichTextBlock {
	RichTextBlock(bool paragraph = true): paragraph(paragraph) {}
	enum Property {
			ForegroundBrush = QTextFormat::ForegroundBrush,
			FontPixelSize = QTextFormat::FontPixelSize,
			FontWeight = QTextFormat::FontWeight,
			FontFamily = QTextFormat::FontFamily,
			FontItalic = QTextFormat::FontItalic,
			FontUnderline = QTextFormat::FontUnderline,
			FontStrikeOut = QTextFormat::FontStrikeOut
	};
	typedef QMap<int, QVariant> Style;
	struct Format {
		void mergeStyle(const Style &style) {
			for (auto it = style.begin(); it != style.end(); ++it)
				this->style[it.key()] = it.value();
		}
		Style style; int begin, end;
	};
	struct Ruby { Style rb_style; QString rt; int rb_begin, rb_end; };
	inline bool hasWords() const {
		for (auto &c : text) {
			if (!RichTextHelper::isSeparator(c.unicode()))
				return true;
		}
		return false;
	}
	QVector<Format> formats; QString text; bool paragraph; QVector<Ruby> rubies;
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

#endif // RICHTEXTBLOCK_HPP
