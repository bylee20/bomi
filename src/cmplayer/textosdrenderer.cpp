#include "textosdrenderer.hpp"
#include "richstring.hpp"
#include <QtGui/QTextDocument>
#include <QtCore/QRegExp>
#include <QtCore/QVector>
#include <QtGui/QPainter>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <qmath.h>
#include "osdstyle.hpp"
#include <QtCore/QMutex>
#include <QtGui/QImage>

class SineCosine {
public:
	static void init() {
		static bool init = false;
		if (!init) {
			static double Sine[count];
			static double Cosine[count];
			const double factor = M_PI*2.0/double(count);
			for (int i=0; i<count; ++i) {
				const double theta = double(i)*factor;
				Sine[i] = qSin(theta);
				Cosine[i] = qCos(theta);
			}
			sine = Sine;
			cosine = Cosine;
		}
	}
	static const double *sine;
	static const double *cosine;
	static const int count = 12;
};

const double *SineCosine::sine = 0;
const double *SineCosine::cosine = 0;

struct TextDrawer {
	enum Type {Pixmap, Image};
	TextDrawer(Type type, const OsdStyle *style, QTextDocument *doc)
	: m_type(type), m_style(style), m_doc(doc), m_border(-1) {}
	QSizeF size() const {return m_size;}
	void update(double border, double width, const RichString &text) {
		if (!qFuzzyCompare(m_border, border)) {
			m_border = border;
			m_points.resize(SineCosine::count);
			for (int i=0; i<SineCosine::count; ++i) {
				m_points[i].setX(m_border*SineCosine::sine[i]);
				m_points[i].setY(m_border*SineCosine::cosine[i]);
			}
		}
		m_text = text;
		m_doc->setHtml(text.toString());
		double w = m_border*2.0;
		double h = m_border*2.0;
		if (m_style->has_shadow) {
			const double ax = qAbs(m_style->shadow_offset.x());
			w += ax + m_style->shadow_blur;
			if (ax < m_style->shadow_blur)
				w += m_style->shadow_blur - ax;
			const double ay = qAbs(m_style->shadow_offset.y());
			w += ay + m_style->shadow_blur;
			if (ay < m_style->shadow_blur)
				w += m_style->shadow_blur - ay;
		}
		m_doc->setTextWidth(width - w);
		if (width > 0.0) {
			m_size.setWidth(m_doc->idealWidth() + w);
			m_size.setHeight(m_doc->size().height() + h);
		} else
			m_size = QSizeF();
		postUpdate();
	}
	virtual ~TextDrawer() {}
	virtual void draw(QPainter *painter, const QPointF &pos) = 0;
	Type type() const {return m_type;}
protected:
	Type m_type;
	virtual void postUpdate() {}
	QVector<QPointF> m_points;
	void drawTextTo(QPainter *painter, const QPointF &origin, const QColor &color, bool overwrite = true) {
		painter->save();
		painter->translate(origin);
		m_doc->setHtml(coloredHtml(color, overwrite));
		m_doc->drawContents(painter);
		painter->restore();
	}
	void drawThickTo(QPaintDevice *dev, QPixmap &buffer
			, const QPointF &origin, const QPointF &offset, const QColor &color) {
		buffer.fill(Qt::transparent);
		QPainter painter(&buffer);
		drawTextTo(&painter, origin, color);
		painter.end();
		painter.begin(dev);
		painter.translate(offset);
		for (int i=0; i<m_points.size(); ++i)
			painter.drawPixmap(m_points[i], buffer);
	}
	void drawThickTo(QPaintDevice *dev, QImage &buffer
			, const QPointF &origin, const QPointF &offset, const QColor &color) {
		buffer.fill(0x0);
		QPainter painter(&buffer);
		drawTextTo(&painter, origin, color);
		painter.end();
		painter.begin(dev);
		painter.translate(offset);
		for (int i=0; i<m_points.size(); ++i)
			painter.drawImage(m_points[i], buffer);
	}
	QString coloredHtml(const QColor &color, bool overwrite) const {
		QString html = QString("<font color='%1'>").arg(color.name());
		if (overwrite) {
			static const QRegExp rxColor("\\s+[cC][oO][lL][oO][rR]\\s*=\\s*[^>\\s\\t]+");
			html += QString(m_text.toString()).remove(rxColor);
		} else
			html += m_text.toString();
		html += "</font>";
		return html;
	}
	QPointF origin() const {
		double x = 0.0;
		const Qt::Alignment alignment = m_doc->defaultTextOption().alignment();
		if (alignment & Qt::AlignLeft) {
			x += m_border;
			if (m_style->has_shadow) {
				if (m_style->shadow_offset.x() < 0)
					x += -m_style->shadow_offset.x() + m_style->shadow_blur;
				else if (m_style->shadow_blur > m_style->shadow_offset.x())
					x += m_style->shadow_blur - m_style->shadow_offset.x();
			}
		} else if (alignment & Qt::AlignRight) {
			x -= m_border;
			if (m_style->has_shadow) {
				if (m_style->shadow_offset.x() > 0)
					x -= m_style->shadow_offset.x() + m_style->shadow_blur;
				else if (m_style->shadow_blur > -m_style->shadow_offset.x())
					x -= m_style->shadow_blur - -m_style->shadow_offset.x();
			}
			x -= (m_doc->textWidth() - m_doc->idealWidth());
		} else
			x -= (m_doc->textWidth() - m_doc->idealWidth())*0.5;
		double y = m_border;
		if (m_style->has_shadow) {
			if (m_style->shadow_offset.y() < 0)
				y += -m_style->shadow_offset.y() + m_style->shadow_blur;
			else if (m_style->shadow_blur > m_style->shadow_offset.y())
				y += m_style->shadow_blur - m_style->shadow_offset.y();
		}
		return QPointF(x, y);
	}
	const OsdStyle *m_style;
	QTextDocument *m_doc;
	double m_border;
	QSizeF m_size;
	RichString m_text;
};

struct CachedTextDrawer : public TextDrawer {
	CachedTextDrawer(Type type, const OsdStyle *style, QTextDocument *doc)
	: TextDrawer(type, style, doc) {}
protected:
//	virtual void cache() = 0;

};

struct PixmapTextDrawer : public CachedTextDrawer { // no blur
	PixmapTextDrawer(const OsdStyle *style, QTextDocument *doc)
	: CachedTextDrawer(Pixmap, style, doc) {}
	void draw(QPainter *painter, const QPointF &pos) {
		painter->drawPixmap(pos, m_cached);
	}
	const QPixmap &cached() const {return m_cached;}
private:
	QPixmap m_cached;
	void postUpdate() {
		const QSize size = OsdRenderer::cachedSize(m_size);
		if (size != m_cached.size())
			m_cached = QPixmap(size);
		cache();
	}
	QPixmap m_interm1, m_interm2;
	void drawThick(const QPointF &origin, const QPointF &offset, const QColor &color) {
		m_interm1.fill(Qt::transparent);
		drawThickTo(&m_interm1, m_interm2, origin, offset, color);
		QPainter painter(&m_cached);
		painter.setOpacity(color.alphaF());
		painter.drawPixmap(QPointF(0, 0), m_interm1);
	}
	void cache() {
		m_cached.fill(Qt::transparent);
		if (m_cached.isNull() || m_size.isEmpty() || m_text.isEmpty())
			return;
		if (m_interm1.size() != m_cached.size())
			m_interm2 = m_interm1 = QPixmap(m_cached.size());
		const QPointF origin = this->origin();
		if (m_style->has_shadow)
			drawThick(origin, m_style->shadow_offset, m_style->shadow_color);
		drawThick(origin, QPointF(0, 0), m_style->color_bg);
		QPainter painter(&m_cached);
		drawTextTo(&painter, origin, m_style->color_fg, false);
	}
};

struct ImageTextDrawer : public CachedTextDrawer {
	ImageTextDrawer(const OsdStyle *style, QTextDocument *doc)
	: CachedTextDrawer(Image, style, doc) {}
	void draw(QPainter *painter, const QPointF &pos) {
		painter->drawImage(pos, m_cached);
	}
private:
	class FastAlphaBlur {
	public:
		FastAlphaBlur(): radius(-1) {}
		// copied from openframeworks superfast blur and modified
		void applyTo(QImage &mask, const QColor &color, int radius) {
			Q_ASSERT(mask.format() == QImage::Format_ARGB32_Premultiplied);
			if (radius < 1 || mask.isNull())
				return;
			setSize(mask.size());
			setRadius(radius);
			const int w = s.width();
			const int h = s.height();

			uchar *a = valpha.data();
			const uchar *inv = vinv.constData();
			int *min = vmin.data();
			int *max = vmax.data();

			const int xmax = mask.width()-1;
			for (int x=0; x<w; ++x) {
				min[x] = qMin(x + radius + 1, xmax);
				max[x] = qMax(x - radius, 0);
			}

			const uchar *c_bits = mask.constBits()+3;
			uchar *it = a;
			for (int y=0; y<h; ++y, c_bits += (mask.width() << 2)) {
				int sum = 0;
				for(int i=-radius; i<=radius; ++i)
					sum += c_bits[qBound(0, i, xmax) << 2];
				for (int x=0; x<w; ++x, ++it) {
					sum += c_bits[min[x] << 2];
					sum -= c_bits[max[x] << 2];
					*it = inv[sum];
				}
			}

			const int ymax = mask.height()-1;
			for (int y=0; y<h; ++y){
				min[y] = qMin(y + radius + 1, ymax)*w;
				max[y] = qMax(y - radius, 0)*w;
			}

			uchar *bits = mask.bits();
			const double r = color.redF();
			const double g = color.greenF();
			const double b = color.blueF();
			const double coef = color.alphaF();
			const uchar *c_it = a;
			for (int x=0; x<w; ++x, ++c_it){
				int sum = 0;
				int yp = -radius*w;
				for(int i=-radius; i<=radius; ++i, yp += w)
					sum += c_it[qMax(0, yp)];
				uchar *p = bits + (x << 2) - 1;
				for (int y=0; y<h; ++y, p += (xmax << 2)){
					const uchar a = inv[sum];
					*(++p) = a*b*coef;
					*(++p) = a*g*coef;
					*(++p) = a*r*coef;
					*(++p) = a*coef;
					sum += c_it[min[y]];
					sum -= c_it[max[y]];
				}
			}
		}

	private:
		void setSize(const QSize &size) {
			if (size != s) {
				s = size;
				if (!s.isEmpty()) {
					vmin.resize(qMax(s.width(), s.height()));
					vmax.resize(vmin.size());
					valpha.resize(s.width()*s.height());
				}
			}
		}
		void setRadius(const int radius) {
			if (this->radius != radius) {
				this->radius = radius;
				if (radius > 0) {
					const int range = (radius << 1) + 1;
					vinv.resize(range << 8);
					for (int i=0; i<vinv.size(); ++i)
						vinv[i] = i/range;
				}
			}
		}
		QSize s;
		QVector<int> vmin, vmax;
		QVector<uchar> valpha, vinv;
		int radius;
	};
	FastAlphaBlur blur;
	QImage m_interm;
	QImage m_cached;
	void postUpdate() {
		const QSize size = OsdRenderer::cachedSize(m_size);
		if (size != m_cached.size()) {
			m_cached = QImage(size, QImage::Format_ARGB32_Premultiplied);
			m_interm = QImage(size, QImage::Format_ARGB32_Premultiplied);
		}
		cache();
	}
	void cache() {
		m_cached.fill(0x0);
		if (m_cached.isNull() || m_size.isEmpty() || m_text.isEmpty())
			return;
		const QPointF origin = this->origin();
		drawThickTo(&m_cached, m_interm, origin, QPointF(0, 0), m_style->color_bg);
		QPainter painter;
		if (m_style->has_shadow) {
			m_interm = m_cached;
			blur.applyTo(m_cached, m_style->shadow_color, m_style->shadow_blur);
			painter.begin(&m_cached);
			painter.drawImage(m_style->shadow_offset, m_interm);
		} else
			painter.begin(&m_cached);
		drawTextTo(&painter, origin, m_style->color_fg, false);
	}
};


struct TextOsdRenderer::Data {
	Qt::Alignment alignment;
	OsdStyle style;
	QTextDocument doc;
	RichString text;
	double bw, top, bottom, left, right;
	QTimer clearer;
	QSizeF bg;
	CachedTextDrawer *drawer;
	void update_drawer() {drawer->update(bw, bg.width() - left - right, text);}
};

TextOsdRenderer::TextOsdRenderer(Qt::Alignment align): d(new Data) {
	SineCosine::init();
	d->doc.setUseDesignMetrics(true);
	d->alignment = 0;
	d->drawer = new PixmapTextDrawer(&d->style, &d->doc);
	d->bw = 1.0;
	d->top = d->bottom = d->left = d->right = 0.0;
	OsdStyle style = this->style();
	style.auto_size = OsdStyle::AutoSize::Width;
	setStyle(style);
	setAlignment(align);
	d->clearer.setSingleShot(true);
	connect(&d->clearer, SIGNAL(timeout()), this, SLOT(clear()));
}

TextOsdRenderer::~TextOsdRenderer() {
	delete d->drawer;
	delete d;
}

Qt::Alignment TextOsdRenderer::alignment() const {
	return d->alignment;
}

void TextOsdRenderer::setAlignment(Qt::Alignment alignment) {
	if (alignment != d->alignment) {
		QTextOption option = d->doc.defaultTextOption();
		option.setAlignment(d->alignment = alignment);
		d->doc.setDefaultTextOption(option);
		emit needToRerender();
	}
}

const OsdStyle &TextOsdRenderer::style() const {
	return d->style;
}

void TextOsdRenderer::setStyle(const OsdStyle &style) {
	d->style = style;
	QTextOption option = d->doc.defaultTextOption();
	option.setWrapMode(style.wrap_mode);
	d->doc.setDefaultTextOption(option);
	updateFont();
	if (d->style.has_shadow && d->style.shadow_blur > 0) {
		if (d->drawer->type() != TextDrawer::Image) {
			delete d->drawer;
			d->drawer = new ImageTextDrawer(&d->style, &d->doc);
		}
	} else if (d->drawer->type() != TextDrawer::Pixmap) {
		delete d->drawer;
		d->drawer = new PixmapTextDrawer(&d->style, &d->doc);
	}
	d->update_drawer();
	emit sizeChanged(d->drawer->size());
	emit styleChanged(d->style);
}

void TextOsdRenderer::render(QPainter *painter, const QPointF &pos) {
	d->drawer->draw(painter, pos);
}

void TextOsdRenderer::showText(const RichString &text, int last) {
	d->clearer.stop();
	d->text = text;
	d->update_drawer();
	emit sizeChanged(d->drawer->size());
	if (last >= 0)
		d->clearer.start(last);
}

RichString TextOsdRenderer::text() const {
	return d->text;
}

QPointF TextOsdRenderer::posHint() const {
	const QSizeF s = size();
	double x = 0.0;
	double y = 0.0;
	if (d->alignment & Qt::AlignBottom) {
		y = qMax(0.0, d->bg.height()*(1.0 - d->bottom) - s.height());
	} else if (d->alignment & Qt::AlignVCenter)
		y = (d->bg.height() - s.height())*0.5;
	else {
		y = d->bg.height()*d->top;
		if (y + s.height() > d->bg.height())
			y = d->bg.height() - s.height();
	}
	if (d->alignment & Qt::AlignHCenter)
		x = (d->bg.width() - s.width())*0.5;
	else if (d->alignment & Qt::AlignRight)
		x = d->bg.width()*(1.0 - d->right) - s.width();
	else
		x = d->bg.width()*d->left;
	return QPointF(x, y);
}

QSizeF TextOsdRenderer::size() const {
	return d->drawer->size();
}

void TextOsdRenderer::updateFont() {
	int px = 0;
	const OsdStyle::AutoSize size = style().auto_size;
	if (size == OsdStyle::AutoSize::Diagonal)
		px = qRound(qSqrt(d->bg.height()*d->bg.height() + d->bg.width()*d->bg.width()) * style().text_scale);
	else if (size == OsdStyle::AutoSize::Width)
		px = qRound(d->bg.width()*style().text_scale);
	else
		px = qRound(d->bg.height()*style().text_scale);
	px = qMax(1, px);
	d->bw = qMax(style().border_width*px, 1.0);
	QFont font = style().font;
	font.setPixelSize(px);
	d->doc.setDefaultFont(font);
}

void TextOsdRenderer::updateBackgroundSize(const QSizeF &size) {
	if (d->bg != size) {
		d->bg = size;
		updateFont();
		d->update_drawer();
		emit sizeChanged(d->drawer->size());
	}
}

void TextOsdRenderer::clear() {
	showText(RichString());
}

void TextOsdRenderer::setMargin(double top, double bottom, double right, double left) {
	d->top = top;
	d->bottom = bottom;
	if (d->right != right || d->left != left) {
		d->right = right;
		d->left = left;
		d->update_drawer();
		emit sizeChanged(d->drawer->size());
	} else
		emit needToRerender();
}
