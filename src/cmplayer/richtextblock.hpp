#ifndef RICHTEXTBLOCK_HPP
#define RICHTEXTBLOCK_HPP

#include "stdafx.hpp"

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
	QVector<Format> formats; QString text; bool paragraph; QVector<Ruby> rubies;
};

#endif // RICHTEXTBLOCK_HPP
