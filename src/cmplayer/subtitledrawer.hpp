#ifndef SUBTITLEDRAWER_HPP
#define SUBTITLEDRAWER_HPP

#include "richtextdocument.hpp"
#include "subtitlestyle.hpp"
#include "subtitle.hpp"

struct Margin {
	Margin() {}
	Margin(const QPointF &tl, const QPointF &br)
		: top(tl.y()), right(br.x()), bottom(br.y()), left(tl.x()) {}
	double top = 0.0, right = 0.0, bottom = 0.0, left = 0.0;
};

class SubCompImage : public QImage {
public:
	SubCompImage(const SubComp *comp, SubComp::const_iterator it, void *creator)
	: m_comp(comp), m_it(it), m_creator(creator) { if (m_it != comp->end()) m_text = *m_it; }
	SubCompImage(const SubComp *comp): m_comp(comp) { if (comp) m_it = m_comp->end(); }
	SubComp::const_iterator iterator() const { return m_it; }
	const QPointF &shadowOffset() const { return m_shadow; }
	const RichTextDocument &text() const { return m_text; }
	const SubComp *component() const { return m_comp; }
	QSize layoutSize() const { return size()/devicePixelRatio(); }
	bool isValid() const { return m_comp && m_it != m_comp->end(); }
	void *creator() const { return m_creator; }
private:
	friend class SubtitleDrawer;
	const SubComp *m_comp = nullptr;
	SubComp::const_iterator m_it; RichTextDocument m_text;
	QPointF m_shadow;
	void *m_creator = nullptr;
};

class SubtitleDrawer {
public:
	void setStyle(const SubtitleStyle &style);
	void setAlignment(Qt::Alignment alignment) {
		m_back.setAlignment(m_alignment = alignment);
		m_front.setAlignment(alignment);
	}
	void setMargin(const Margin &margin) { m_margin = margin; }
	bool hasDrawn() const {return m_drawn;}
	bool draw(QImage &image, QPointF &shadow, const RichTextDocument &text, const QRectF &area, double dpr = 1.0);
	bool draw(SubCompImage &pic, const QRectF &area, double dpr = 1.0) {
		return draw(pic, pic.m_shadow, pic.m_text, area, dpr);
	}
	QPointF pos(const QSizeF &image, const QRectF &area) const;
	Qt::Alignment alignment() const { return m_alignment; }
	const Margin &margin() const { return m_margin; }
	const SubtitleStyle &style() const {return m_style;}
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
private:
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
	RichTextDocument m_front, m_back;
	Margin m_margin;
	Qt::Alignment m_alignment;
	bool m_drawn = false;
};

#endif // SUBTITLEDRAWER_HPP
