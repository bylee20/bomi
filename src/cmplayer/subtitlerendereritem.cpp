#include "subtitlerendereritem.hpp"
#include "richtextdocument.hpp"

struct SubtitleRendererItem::Data {
	RichTextDocument front, back;
	double top = 0.0, bottom = 0.0, left = 0.0, right = 0.0;
	bool redraw = false;
	int loc_tex = 0, loc_shadowColor = 0, loc_shadowOffset = 0, loc_dxdy = 0;
	QImage image, next, blank = {1, 1, QImage::Format_ARGB32_Premultiplied};
	QRectF screen = {0, 0, 0, 0};
	QPointF shadowOffset = {0, 0};
	bool empty = true;
	QSize textureSize = {0, 0};
};

SubtitleRendererItem::SubtitleRendererItem(QQuickItem *parent)
: TextureRendererItem(1, parent), d(new Data) {
	d->blank.fill(0x0);
	updateAlignment();
	updateStyle();
	prepare();
	update();
}

SubtitleRendererItem::~SubtitleRendererItem() {
	delete d;
}

QRectF SubtitleRendererItem::drawArea() const {
	return m_letterbox ? boundingRect() : d->screen;
}

void SubtitleRendererItem::setScreenRect(const QRectF &screen) {
	if (d->screen != screen) {
		d->screen = screen;
		if (!m_letterbox) {
			setGeometryDirty();
			prepare();
			update();
		}
	}
}

void SubtitleRendererItem::updateStyle() {
	auto update = [this] (RichTextDocument &doc) {
		doc.setFontPixelSize(m_style->font()->height());
		doc.setWrapMode(m_style->wrap_mode);
		doc.setFormat(QTextFormat::ForegroundBrush, QBrush(m_style->font()->color()));
		doc.setFormat(QTextFormat::FontFamily, m_style->font()->family());
		doc.setFormat(QTextFormat::FontUnderline, m_style->font()->underline());
		doc.setFormat(QTextFormat::FontStrikeOut, m_style->font()->strikeOut());
		doc.setFormat(QTextFormat::FontWeight, m_style->font()->weight());
		doc.setFormat(QTextFormat::FontItalic, m_style->font()->italic());
		doc.setLeading(m_style->spacing()->line(), m_style->spacing()->paragraph());
	};
	update(d->front);	update(d->back);		setGeometryDirty();
	d->back.setTextOutline(m_style->outline()->color(), m_style->font()->height()*m_style->outline()->width()*2.0);
}

void SubtitleRendererItem::updateAlignment() {
	d->back.setAlignment(m_alignment);
	d->front.setAlignment(m_alignment);
}

void SubtitleRendererItem::setText(const RichTextDocument &doc) {
	d->front = doc.blocks();
	d->back = doc.blocks();
	prepare();
	update();
}

void SubtitleRendererItem::setAlignment(Qt::Alignment alignment) {
	if (alignment != m_alignment) {
		m_alignment = alignment;
		updateAlignment();
		prepare();
		update();
	}
}

double SubtitleRendererItem::scale() const {
	auto area = drawArea();
	const auto policy = m_style->font()->scale();
	double px = m_style->font()->size();
	if (policy == m_style->font()->Diagonal)
		px *= _Diagonal(area.size());
	else if (policy == m_style->font()->Width)
		px *= area.width();
	else
		px *= area.height();
	return px/m_style->font()->height();
}

void SubtitleRendererItem::prepare() {
	d->empty = !d->front.hasWords();
	if (!d->empty) {
		double scale = this->scale();
		d->front.updateLayoutInfo();
		d->back.updateLayoutInfo();
		d->front.doLayout(width()/scale);
		d->back.doLayout(width()/scale);

		d->shadowOffset = m_style->shadow()->offset()*m_style->font()->height()*scale;
		d->image = QImage(width() + qAbs(d->shadowOffset.x()), d->front.naturalSize().height()*scale + qAbs(d->shadowOffset.y()), QImage::Format_ARGB32_Premultiplied);
		d->empty = d->image.isNull();
		if (!d->empty) {
			d->image.fill(0x0);
			QPainter painter(&d->image);
			if (d->shadowOffset.x() < 0)
				painter.translate(-d->shadowOffset.x(), 0);
			if (d->shadowOffset.y() < 0)
				painter.translate(0, -d->shadowOffset.x());
			painter.scale(scale, scale);
			d->back.draw(&painter, QPointF(0, 0));
			d->front.draw(&painter, QPointF(0, 0));
			painter.end();
			d->redraw = true;
			d->shadowOffset.rx() /= (double)d->image.width();
			d->shadowOffset.ry() /= (double)d->image.height();
		}
	}
	setVisible(!d->empty);
	if (d->empty)
		d->shadowOffset = QPointF(0, 0);
}


void SubtitleRendererItem::link(QOpenGLShaderProgram *program) {
	TextureRendererItem::link(program);
	d->loc_tex = program->uniformLocation("tex");
	d->loc_dxdy = program->uniformLocation("dxdy");
	d->loc_shadowColor = program->uniformLocation("shadowColor");
	d->loc_shadowOffset = program->uniformLocation("shadowOffset");
}

void SubtitleRendererItem::bind(const RenderState &state, QOpenGLShaderProgram *program) {
	TextureRendererItem::bind(state, program);
	program->setUniformValue(d->loc_tex, 0);
	program->setUniformValue(d->loc_shadowColor, m_style->shadow()->color());
	program->setUniformValue(d->loc_shadowOffset, d->shadowOffset);
	program->setUniformValue(d->loc_dxdy, 1.0/(d->image.width()), 1.0/(d->image.height()));
	auto f = QOpenGLContext::currentContext()->functions();
	f->glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture(0));
}

void SubtitleRendererItem::updateTexturedPoint2D(TexturedPoint2D *tp) {
	const auto area = drawArea();
	const QSizeF size = d->image.size();
	QPointF pos(0.0, 0.0);
	if (m_alignment & Qt::AlignBottom) {
		pos.ry() = qMax(0.0, area.height()*(1.0 - d->bottom) - size.height());
	} else if (m_alignment & Qt::AlignVCenter)
		pos.ry() = (area.height() - size.height())*0.5;
	else {
		pos.ry() = area.height()*d->top;
		if (pos.y() + size.height() > area.height())
			pos.ry() = area.height() - size.height();
	}

	if (m_alignment & Qt::AlignHCenter)
		pos.rx() = (area.width() - size.width())*0.5;
	else if (m_alignment & Qt::AlignRight)
		pos.rx() = area.width()*(1.0 - d->right) - size.width();
	else
		pos.rx() = area.width()*d->left;

	pos += area.topLeft();

	set(tp, QRectF(pos, size), QRectF(0, 0, 1, 1));
}

void SubtitleRendererItem::initializeTextures() {
	if (!d->empty) {
		glBindTexture(GL_TEXTURE_2D, texture(0));
		glTexImage2D(GL_TEXTURE_2D, 0, 4, d->image.width(), d->image.height(), 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, d->image.bits());
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	setGeometryDirty();
}

void SubtitleRendererItem::beforeUpdate() {
	if (d->redraw) {
		initializeTextures();
		setGeometryDirty();
		d->redraw = false;
	}
}

void SubtitleRendererItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
	TextureRendererItem::geometryChanged(newGeometry, oldGeometry);
	if (m_letterbox) {
		setGeometryDirty();
		prepare();
		update();
	}
}

const char *SubtitleRendererItem::fragmentShader() const {
	const char *shader = (R"(
		uniform sampler2D tex;
		uniform vec2 dxdy;
		uniform vec2 shadowOffset;
		uniform vec4 shadowColor;
		varying highp vec2 qt_TexCoord;
		void main() {
			vec4 top = texture2D(tex, qt_TexCoord);
			float alpha = texture2D(tex, qt_TexCoord - shadowOffset).a;
			gl_FragColor = top + alpha*shadowColor*(1.0 - top.a);
		}
	)");
	return shader;
}

void SubtitleRendererItem::setMargin(double top, double bottom, double right, double left) {
	d->top = top;
	d->bottom = bottom;
	if (d->right != right || d->left != left) {
		d->right = right;
		d->left = left;
	}
	setGeometryDirty();
	update();
}

void SubtitleRendererItem::setLetterboxHint(bool hint) {
	if (m_letterbox != hint) {
		m_letterbox = hint;
		if (d->screen != boundingRect()) {
			prepare();
			update();
		}
	}
}
