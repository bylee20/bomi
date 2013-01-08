#include "subtitlerendereritem.hpp"
#include "richtextdocument.hpp"

struct SubtitleRendererItem::Data {
	RichTextDocument doc[2];
	QList<RichTextBlock> blocks;
	bool textChanged = true;
	bool optionChanged = true;
	bool formatChanged = true;
	bool styleChanged = true;

	double outline = 1.0, lineLeading = 0.0, paragraphLeading = 0.0;
	double top = 0.0, bottom = 0.0, left = 0.0, right = 0.0;
	int px = 20;

	QImage image;
	bool redraw = false;
	int loc_tex = 0;
};

SubtitleRendererItem::SubtitleRendererItem(QQuickItem *parent)
: TextureRendererItem(1, parent), d(new Data) {
	setText(RichTextDocument("<p> test string </p>"));
}

SubtitleRendererItem::~SubtitleRendererItem() {
	delete d;
}

void SubtitleRendererItem::setText(const RichTextDocument &doc) {
	d->blocks = doc.blocks();
	d->doc[0] = d->doc[1] = doc;
	d->textChanged = true;
//	d->pended = doc.blocks();
//	if (last >= 0)
//		d->clearer.start(last);
	prepare();
	update();
}

void SubtitleRendererItem::setAlignment(Qt::Alignment alignment) {
	if (alignment != m_alignment) {
		m_alignment = alignment;
		d->optionChanged = true;
		update();
	}
}

//void TextOsdRenderer::updateStyle(const OsdStyle &) {
//	updateFont();
//	d->optionChanged = true;
//	d->formatChanged = true;
//	d->styleChanged = true;
//	emit needToRerender();
//}

//void TextOsdRenderer::updateFont() {
//	const auto scale = this->scale();
//	const auto &style = this->style();
//	d->px = qMax(1, qRound(scale*style.size));
//	d->outline = style.has_outline ? scale*style.outline_width : -1.0;
//	d->doc[0].setLeading(style.line_spacing*scale, style.paragraph_spacing*scale);
//	d->doc[1].setLeading(style.line_spacing*scale, style.paragraph_spacing*scale);
//	d->formatChanged = true;
//}

void SubtitleRendererItem::prepare() {
	auto apply = [this] (RichTextDocument &doc) {
		if (d->optionChanged) {
			doc.setAlignment(m_alignment);
			doc.setWrapMode(m_style->wrap_mode);
		}
//		if (d->formatChanged) {
			doc.setFontPixelSize(d->px);
//		}
		if (d->styleChanged) {
			doc.setFormat(QTextFormat::ForegroundBrush, QBrush(m_style->color()));
			doc.setFormat(QTextFormat::FontFamily, m_style->font.family());
			doc.setFormat(QTextFormat::FontUnderline, m_style->font.underline());
			doc.setFormat(QTextFormat::FontStrikeOut, m_style->font.strikeOut());
			doc.setFormat(QTextFormat::FontWeight, m_style->font.weight());
			doc.setFormat(QTextFormat::FontItalic, m_style->font.italic());
		}
	};
	apply(d->doc[0]);	apply(d->doc[1]);
//	if (d->formatChanged || d->styleChanged)
//		d->doc.setTextOutline(m_style->outline()->color(), d->outline*2.0);
//	apply(d->doc[0]);		apply(d->doc[1]);
//	if (d->textChanged) {
//		d->doc[0] = d->pended;
//		d->doc[1] = d->doc[0].blocks();
//	}
	d->textChanged = d->optionChanged = d->formatChanged = d->styleChanged = false;
	d->doc[0].updateLayoutInfo();
	d->doc[1].updateLayoutInfo();
	d->doc[0].doLayout(width());
	d->doc[1].doLayout(width());

//	qDebug() << d->doc[0].naturalSize().toSize();
	d->image = QImage(width(), d->doc[0].naturalSize().height(), QImage::Format_ARGB32_Premultiplied);
	d->image.fill(0x0);
	QPainter painter(&d->image);
	d->doc[0].draw(&painter, QPointF(0, 0));
	painter.end();
	d->redraw = true;
}


void SubtitleRendererItem::link(QOpenGLShaderProgram *program) {
	TextureRendererItem::link(program);
	d->loc_tex = program->uniformLocation("tex");
}

void SubtitleRendererItem::bind(const RenderState &state, QOpenGLShaderProgram *program) {
	TextureRendererItem::bind(state, program);
	program->setUniformValue(d->loc_tex, 0);
	auto f = QOpenGLContext::currentContext()->functions();
	f->glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture(0));
}

void SubtitleRendererItem::updateTexturedPoint2D(TexturedPoint2D *tp) {
	TextureRendererItem::updateTexturedPoint2D(tp);
}

bool SubtitleRendererItem::beforeUpdate() {
	if (d->redraw) {
		glBindTexture(GL_TEXTURE_2D, texture(0));
		glTexImage2D(GL_TEXTURE_2D, 0, 4, d->image.width(), d->image.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, d->image.bits());
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		qDebug() << d->image.size();
		d->redraw = false;
	}
	//		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//		glEnable(GL_BLEND);

	//		glEnableClientState(GL_VERTEX_ARRAY);
	//		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	//		glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);
	//		glVertexPointer(2, GL_FLOAT, 0, vertexCoords);
	//		glDrawArrays(GL_QUADS, 0, 4);
	//		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	//		glDisableClientState(GL_VERTEX_ARRAY);
	//		shader->release();
	//	}
	setGeometryDirty();
	return false;
}

//void TextOsdRenderer::render(QPainter *painter, const QPointF &pos, int layer) {
////	prepareToRender(pos);
//	const QPointF p(pos.x() - posHint().x(), pos.y());
//	if (0 <= layer && layer < 2)
//		d->doc[layer].draw(painter, p);
//}

const char *SubtitleRendererItem::fragmentShader() const {
	const char *shader = (R"(
		uniform sampler2D tex;
//		uniform vec2 dxdy;
//		uniform vec2 shadow_offset;
//		uniform vec4 shadow_color;
		varying highp vec2 qt_TexCoord;
		void main() {
			vec4 top = texture2D(tex, qt_TexCoord);
//			float alpha = texture2D(tex, qt_TexCoord - dxdy*shadow_offset).a;
			gl_FragColor = top;// + alpha*shadow_color*(1.0 - top.a);
//			gl_FragColor.rgba = vec4(1.0, 1.0, 1.0, 1.0);
		}
	)");
	return shader;

}
