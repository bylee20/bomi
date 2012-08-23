#include "textosdrenderer.hpp"
#include "global.hpp"
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtGui/QPainter>
#include "richtextdocument.hpp"

struct TextOsdRenderer::Data {
	QList<RichTextBlock> pended;
	RichTextDocument doc[2];
	QString text;
	Qt::Alignment alignment = 0;
	double outline = 1.0, lineLeading = 0.0, paragraphLeading = 0.0;
	double top = 0.0, bottom = 0.0, left = 0.0, right = 0.0;
	int px = 20;
	QTimer clearer;
	QSizeF renderable;
	bool textChanged = false, optionChanged = false, formatChanged = false, sizeChanged = false, styleChanged = false;
	bool pending = false;
};

TextOsdRenderer::TextOsdRenderer(Qt::Alignment align)
: d(new Data ) {
	setAlignment(align);
	d->clearer.setSingleShot(true);
	connect(&d->clearer, SIGNAL(timeout()), this, SLOT(clear()));
	TextOsdRenderer::updateStyle(style());
}

TextOsdRenderer::~TextOsdRenderer() {
	delete d;
}

Qt::Alignment TextOsdRenderer::alignment() const {
	return d->alignment;
}

void TextOsdRenderer::setAlignment(Qt::Alignment alignment) {
	if (alignment != d->alignment) {
		d->alignment = alignment;
		d->optionChanged = true;
		emit needToRerender();
	}
}

void TextOsdRenderer::updateStyle(const OsdStyle &) {
	updateFont();
	d->optionChanged = true;
	d->formatChanged = true;
	d->styleChanged = true;
	emit needToRerender();
}

void TextOsdRenderer::updateFont() {
	const auto scale = this->scale();
	const auto &style = this->style();
	d->px = qMax(1, qRound(scale*style.size));
	d->outline = style.has_outline ? scale*style.outline_width : -1.0;
	d->doc[0].setLeading(style.line_spacing*scale, style.paragraph_spacing*scale);
	d->doc[1].setLeading(style.line_spacing*scale, style.paragraph_spacing*scale);
	d->formatChanged = true;
}

void TextOsdRenderer::prepareToRender(const QPointF &) {
	auto &style = this->style();
	auto apply = [this, &style] (RichTextDocument &doc) {
		if (d->optionChanged) {
			doc.setAlignment(d->alignment);
			doc.setWrapMode(style.wrap_mode);
		}
		if (d->formatChanged) {
			doc.setFontPixelSize(d->px);
		}
		if (d->styleChanged) {
			doc.setFormat(QTextFormat::ForegroundBrush, QBrush(style.color));
			doc.setFormat(QTextFormat::FontFamily, style.font.family());
			doc.setFormat(QTextFormat::FontUnderline, style.font.underline());
			doc.setFormat(QTextFormat::FontStrikeOut, style.font.strikeOut());
			doc.setFormat(QTextFormat::FontWeight, style.font.weight());
			doc.setFormat(QTextFormat::FontItalic, style.font.italic());
		}
	};
	if (d->formatChanged || d->styleChanged)
		d->doc[1].setTextOutline(style.outline_color, d->outline*2.0);
	apply(d->doc[0]);		apply(d->doc[1]);
	if (d->textChanged) {
		d->doc[0] = d->pended;
		d->doc[1] = d->doc[0].blocks();
	}
	d->textChanged = d->optionChanged = d->formatChanged = d->styleChanged = false;
	d->doc[0].updateLayoutInfo();
	d->doc[1].updateLayoutInfo();
	d->doc[0].doLayout(renderableSize().width());
	d->doc[1].doLayout(renderableSize().width());
}

void TextOsdRenderer::render(QPainter *painter, const QPointF &pos, int layer) {
//	prepareToRender(pos);
	const QPointF p(pos.x() - posHint().x(), pos.y());
	if (0 <= layer && layer < 2)
		d->doc[layer].draw(painter, p);
}

void TextOsdRenderer::show(const RichTextDocument &doc, int last) {
	d->clearer.stop();
	d->pended = doc.blocks();
	d->textChanged = true;
	emit needToRerender();
	if (last >= 0)
		d->clearer.start(last);
}

RichTextDocument TextOsdRenderer::doc() const {
	RichTextDocument doc;
	doc += d->pended;
	return doc;
}

void TextOsdRenderer::show(const QString &text, int last) {
	RichTextDocument doc;
	doc.setText(text);
	show(doc, last);
}

QPointF TextOsdRenderer::posHint() const {
	const QSizeF s = size();
	double x = 0.0;
	double y = 0.0;
	if (d->alignment & Qt::AlignBottom) {
		y = qMax(0.0, d->renderable.height()*(1.0 - d->bottom) - s.height());
	} else if (d->alignment & Qt::AlignVCenter)
		y = (d->renderable.height() - s.height())*0.5;
	else {
		y = d->renderable.height()*d->top;
		if (y + s.height() > d->renderable.height())
			y = d->renderable.height() - s.height();
	}
	if (d->alignment & Qt::AlignHCenter)
		x = (d->renderable.width() - s.width())*0.5;
	else if (d->alignment & Qt::AlignRight)
		x = d->renderable.width()*(1.0 - d->right) - s.width();
	else
		x = d->renderable.width()*d->left;
	return QPointF(x, y);
}

QSizeF TextOsdRenderer::size() const {
	return d->doc[0].naturalSize();
}

bool TextOsdRenderer::updateRenderableSize(const QSizeF &size) {
	if (d->renderable != size) {
		d->renderable = size;
		updateFont();
		emit needToRerender();
		return true;
	} else
		return false;
}

void TextOsdRenderer::clear() {
	d->clearer.stop();
	d->pended.clear();
	d->textChanged = true;
	emit needToRerender();
}

void TextOsdRenderer::setMargin(double top, double bottom, double right, double left) {
	d->top = top;
	d->bottom = bottom;
	if (d->right != right || d->left != left) {
		d->right = right;
		d->left = left;
	}
	emit needToRerender();
}
