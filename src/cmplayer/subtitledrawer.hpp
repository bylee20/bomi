#ifndef SUBTITLEDRAWER_HPP
#define SUBTITLEDRAWER_HPP

#include "richtextdocument.hpp"
#include "subtitlestyle.hpp"

struct Margin { double top = 0.0, right = 0.0, bottom = 0.0, left = 0.0; };

class SubtitleDrawer {
public:
	void setStyle(const SubtitleStyle &style);
	void setAlignment(Qt::Alignment alignment) { back.setAlignment(m_alignment = alignment); front.setAlignment(alignment); }
	void setText(const RichTextDocument &doc) { front = doc.blocks(); back = doc.blocks(); }
	void setMargin(const Margin &margin) { m_margin = margin; }
	bool hasDrawn() const {return m_drawn;}
	bool draw(QImage &image, QSize &imageSize, QPointF &shadowOffset, const QRectF &area, double dpr = 1.0);
	QPointF pos(const QSizeF &image, const QRectF &area) const;
	Qt::Alignment alignment() const { return m_alignment; }
	const Margin &margin() const { return m_margin; }
	const SubtitleStyle &style() const {return m_style;}
	const RichTextDocument &text() const {return front;}
private:
	double scale(const QRectF &area) const {
		const auto policy = m_style.font.scale;
		double px = m_style.font.size;
		if (policy == SubtitleStyle::Font::Scale::Diagonal)
			px *= _Diagonal(area.size());
		else if (policy == SubtitleStyle::Font::Scale::Width)
			px *= area.width();
		else
			px *= area.height();
		return px/m_style.font.height();
	}
	void updateLayoutInfo() { front.updateLayoutInfo(); back.updateLayoutInfo(); }
	void doLayout(double maxWidth) { front.doLayout(maxWidth); back.doLayout(maxWidth); }
	static void updateStyle(RichTextDocument &doc, const SubtitleStyle &style) {
		doc.setFontPixelSize(style.font.height());
		doc.setWrapMode(style.wrapMode);
		doc.setFormat(QTextFormat::ForegroundBrush, QBrush(style.font.color));
		doc.setFormat(QTextFormat::FontFamily, style.font.family());
		doc.setFormat(QTextFormat::FontUnderline, style.font.underline());
		doc.setFormat(QTextFormat::FontStrikeOut, style.font.strikeOut());
		doc.setFormat(QTextFormat::FontWeight, style.font.weight());
		doc.setFormat(QTextFormat::FontItalic, style.font.italic());
		doc.setLeading(style.spacing.line, style.spacing.paragraph);
	}
	SubtitleStyle m_style;
	RichTextDocument front, back;
	Margin m_margin;
	Qt::Alignment m_alignment;
	bool m_drawn = false;
};

#endif // SUBTITLEDRAWER_HPP
