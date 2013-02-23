#ifndef RICHTEXTDOCUMENT_HPP
#define RICHTEXTDOCUMENT_HPP

#include "stdafx.hpp"
#include "richtextblock.hpp"
#include "richtexthelper.hpp"

class RichTextDocument : public RichTextHelper {
public:
	RichTextDocument();
	RichTextDocument(const QString &text);
	RichTextDocument(const RichTextDocument &rhs);
	~RichTextDocument();
	RichTextDocument &operator = (const RichTextDocument &rhs);
	RichTextDocument &operator = (const QList<RichTextBlock> &rhs);
	RichTextDocument &operator += (const RichTextDocument &rhs);
	RichTextDocument &operator += (const QList<RichTextBlock> &rhs);
	inline bool isEmpty() const {return m_blocks.isEmpty();}
	inline int totalLength() const {int ret = 0; for (auto &block : m_blocks) {ret += block.text.size();} return ret;}
	inline bool hasWords() const {for (auto &block : m_blocks) {for (auto &c : block.text) {if (!isSeparator(c.unicode())) return true;}}	return false;}
	inline QString toPlainText() const {QString ret; for (auto &block : m_blocks) {ret += block.text;} return ret;}
	inline const QList<RichTextBlock> &blocks() const {return m_blocks;}
	void setAlignment(Qt::Alignment alignment);
	void setWrapMode(QTextOption::WrapMode wrapMode);
	void setFormat(QTextFormat::Property property, const QVariant &data);
	void draw(QPainter *painter, const QPointF &pos);
	void doLayout(double maxWidth);
	void updateLayoutInfo();
	void setText(const QString &text);
	void setFontPixelSize(int px);
	void setTextOutline(const QColor &color, double width) {setTextOutline(QPen(color, width));}
	void setTextOutline(const QPen &pen);
	QSizeF naturalSize() const {return m_natural.size();}
	void setLeading(double newLine, double paragraph);
private:
	struct Layout {
		QTextLayout *block;
		QVector<QTextLayout*> rubies;
	};

	void freeLayouts();
	inline void setChanged(bool changed) {m_dirty = m_blockChanged = m_formatChanged = m_pxChanged = m_optionChanged = changed;}
	QVector<Layout> m_layouts;
	QList<RichTextBlock> m_blocks;
	QTextOption m_option;
	QTextCharFormat m_format;
	bool m_blockChanged, m_formatChanged, m_optionChanged, m_pxChanged;
	bool m_dirty;
	double m_lineLeading = 0, m_paragraphLeading = 0;
	QRectF m_natural;
};

#endif // RICHTEXTDOCUMENT_HPP
