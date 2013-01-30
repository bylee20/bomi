#include "subtitlerendereritem.hpp"
#include "richtextdocument.hpp"
#include "subtitlemodel.hpp"
#include "subtitlestyle.hpp"

SubtitleRendererItem::Render::Render(const SubtitleComponent &comp)
: comp(&comp), prev(comp.end()), model(new SubtitleComponentModel(&comp)) {}

SubtitleRendererItem::Render::~Render() { delete this->model; }

struct SubtitleRendererItem::Data {
	SubtitleStyle style;
	bool docempty = true;
	RichTextDocument front, back;
	double top = 0.0, bottom = 0.0, left = 0.0, right = 0.0;
	bool redraw = false;
	int loc_tex = 0, loc_shadowColor = 0, loc_shadowOffset = 0, loc_dxdy = 0;
	QImage image, next, blank = {1, 1, QImage::Format_ARGB32_Premultiplied};
	QPointF shadowOffset = {0, 0};
	QSize textureSize = {0, 0};
	QSize imageSize = {0, 0};
	bool selecting = false;
	QMap<QString, int> langMap;
	int language_priority(const Render *r) const {
//		return langMap.value(r->comp->language().id(), -1);
		return langMap.value(r->comp->language(), -1);
	}
};

SubtitleRendererItem::SubtitleRendererItem(QQuickItem *parent)
: TextureRendererItem(1, parent), d(new Data) {
	d->blank.fill(0x0);
	updateAlignment();	updateStyle();	prepare();	update();
}

SubtitleRendererItem::~SubtitleRendererItem() {
	delete d;
}

void SubtitleRendererItem::setPriority(const QStringList &priority) {
	for (int i=0; i< priority.size(); ++i)
		d->langMap[priority[i]] = priority.size()-i;
}

void SubtitleRendererItem::setStyle(const SubtitleStyle &style) {
	d->style = style;
	updateStyle();
}

void SubtitleRendererItem::updateStyle() {
	auto update = [this] (RichTextDocument &doc) {
		doc.setFontPixelSize(d->style.font.height());
		doc.setWrapMode(d->style.wrapMode);
		doc.setFormat(QTextFormat::ForegroundBrush, QBrush(d->style.font.color));
		doc.setFormat(QTextFormat::FontFamily, d->style.font.family());
		doc.setFormat(QTextFormat::FontUnderline, d->style.font.underline());
		doc.setFormat(QTextFormat::FontStrikeOut, d->style.font.strikeOut());
		doc.setFormat(QTextFormat::FontWeight, d->style.font.weight());
		doc.setFormat(QTextFormat::FontItalic, d->style.font.italic());
		doc.setLeading(d->style.spacing.line, d->style.spacing.paragraph);
	};
	update(d->front);	update(d->back);		setGeometryDirty();
	if (d->style.outline.enabled)
		d->back.setTextOutline(d->style.outline.color, d->style.font.height()*d->style.outline.width*2.0);
	else
		d->back.setTextOutline(Qt::NoPen);
	prepare();
	this->update();
}

void SubtitleRendererItem::updateAlignment() {
	d->back.setAlignment(m_alignment);
	d->front.setAlignment(m_alignment);
}

void SubtitleRendererItem::setTopAlignment(bool top) {
	if (_Change(m_top, top)) {
		if (_Change(m_alignment, Qt::AlignHCenter | (m_top ? Qt::AlignTop : Qt::AlignBottom))) {
			updateAlignment();
			prepare();
			update();
		}
		setPos(1.0 - m_pos);
	}
}

void SubtitleRendererItem::setText(const RichTextDocument &doc) {
	d->front = doc.blocks();
	d->back = doc.blocks();
	prepare();
	update();
}

double SubtitleRendererItem::scale() const {
	auto area = drawArea();
	const auto policy = d->style.font.scale;
	double px = d->style.font.size;
	if (policy == SubtitleStyle::Font::Scale::Diagonal)
		px *= _Diagonal(area.size());
	else if (policy == SubtitleStyle::Font::Scale::Width)
		px *= area.width();
	else
		px *= area.height();
	return px/d->style.font.height();
}

void SubtitleRendererItem::prepare() {
	d->docempty = !d->front.hasWords();
	if (!d->docempty) {
		double scale = this->scale();
		d->front.updateLayoutInfo();
		d->back.updateLayoutInfo();
		d->front.doLayout(width()/scale);
		d->back.doLayout(width()/scale);

		d->shadowOffset = d->style.shadow.offset*d->style.font.height()*scale;
		d->imageSize.rwidth() = width();
		d->imageSize.rheight() = d->front.naturalSize().height()*scale + qAbs(d->shadowOffset.y());

		double dpr = 1.0;
		if (window() && window()->screen()) {
			auto screen = window()->screen();
			dpr = screen->logicalDotsPerInch()/screen->physicalDotsPerInch();
			if (dpr < 1.0)
				dpr = 1.0;
		}

		d->image = QImage(d->imageSize*dpr, QImage::Format_ARGB32_Premultiplied);
		d->docempty = d->image.isNull();
		d->image.setDevicePixelRatio(dpr);
		if (!d->docempty) {
			d->image.fill(0x0);
			QPainter painter(&d->image);
			if (d->shadowOffset.y() < 0)
				painter.translate(0, -d->shadowOffset.y());
			painter.scale(scale*dpr, scale*dpr);
			d->back.draw(&painter, QPointF(0, 0));
			d->front.draw(&painter, QPointF(0, 0));
			painter.end();
			d->redraw = true;
			d->shadowOffset.rx() /= (double)d->imageSize.width();
			d->shadowOffset.ry() /= (double)d->imageSize.height();
		}
	}
	setVisible(!d->docempty);
	if (d->docempty)
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
	program->setUniformValue(d->loc_shadowColor, d->style.shadow.color);
	program->setUniformValue(d->loc_shadowOffset, d->shadowOffset);
	program->setUniformValue(d->loc_dxdy, 1.0/(d->image.width()), 1.0/(d->image.height()));
	auto f = QOpenGLContext::currentContext()->functions();
	f->glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture(0));
}

void SubtitleRendererItem::updateTexturedPoint2D(TexturedPoint2D *tp) {
	const auto area = drawArea();
	const QSizeF size = d->imageSize;
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
	if (!d->docempty) {
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

void SubtitleRendererItem::render(int ms) {
	m_ms = ms;
	if (!m_visible || m_compempty || ms == 0)
		return;
	bool changed = false;
	for (auto render : m_order) {
		auto it = render->comp->start(ms - m_delay, m_fps);
		if (it != render->prev) {
			render->prev = it;
			render->model->setCurrentCaption(&(*it));
			changed = true;
		}
	}
	if (changed) {
		RichTextDocument doc;
		for (auto render : m_order) {
			if (render->prev != render->comp->end())
				doc += render->prev.value();
		}
		setText(doc);
	}
}

void SubtitleRendererItem::select(int idx, bool selected) {
	if (0 <= idx && idx < m_loaded.size() && m_loaded[idx].isSelected() != selected) {
		m_loaded[idx].selection() = selected;
		const auto comp = &m_loaded[idx].component();
		Render *render = nullptr;
		for (auto it = m_order.begin(); it!= m_order.end(); ++it) {
			if ((*it)->comp == comp) {
				render = *it;
				m_order.erase(it);
				render->prev = comp->end();
				break;
			}
		}
		if (selected) {
			if (!render)
				render = new Render(*comp);
			m_order.prepend(render);
			for (auto it = m_order.begin(); it != m_order.end(); ) {
				Render *&prev = *it;
				if (++it == m_order.end())
					break;
				Render *&next = *it;
				if (d->language_priority(prev) >= d->language_priority(next))
					break;
				qSwap(prev, next);
			}
		}
		if (!d->selecting)
			applySelection();
	}
}

bool SubtitleRendererItem::load(const QString &fileName, const QString &enc, bool select) {
	const int idx = m_loaded.size();
	Subtitle sub;
	if (!sub.load(fileName, enc))
		return false;
	for (int i=0; i<sub.size(); ++i)
		m_loaded.append(LoadedSubtitle(sub[i]));
	if (select) {
		d->selecting = true;
		for (int i=m_loaded.size()-1; i>=idx; --i) {
			this->select(i, select);
		}
		d->selecting = false;
		applySelection();
	}
	return true;
}

int SubtitleRendererItem::start(int time) const {
	int ret = -1;
	for (auto it = m_order.begin(); it != m_order.end(); ++it) {
		const auto &comp = *(*it)->comp;
		const auto sIt = comp.start(time - m_delay, m_fps);
		if (sIt != comp.end())
			ret = qMax(ret, comp.isBasedOnFrame() ? comp.msec(sIt.key(), m_fps) : sIt.key());
	}
	return ret;
}

int SubtitleRendererItem::finish(int time) const {
	int ret = -1;
	for (auto it = m_order.begin(); it != m_order.end(); ++it) {
		const auto &comp = *(*it)->comp;
		const auto sIt = comp.finish(time - m_delay, m_fps);
		if (sIt != comp.end()) {
			const int t = comp.isBasedOnFrame() ? comp.msec(sIt.key(), m_fps) : sIt.key();
			ret = ret == -1 ? t : qMin(ret, t);
		}
	}
	return ret;
}

int SubtitleRendererItem::current() const {
	int time = -1;
	for (auto render : m_order) {
		if (render->prev != render->comp->end() && render->prev->hasWords()) {
			if (time < 0)
				time = render->prev.key();
			else if (render->prev.key() > time)
				time = render->prev.key();
		}
	}
	return time;
}

int SubtitleRendererItem::previous() const {
	int time = -1;
	for (auto render : m_order) {
		if (render->prev != render->comp->end()) {
			auto it = render->prev;
			while (it != render->comp->begin()) {
				if ((--it)->hasWords()) {
					if (time < 0)
						time = it.key();
					else if (it.key() > time)
						time = it.key();
					break;
				}
			}
		}
	}
	return time;
}

int SubtitleRendererItem::next() const {
	int time = -1;
	for (auto render : m_order) {
		if (render->prev != render->comp->end()) {
			auto it = render->prev;
			while (++it != render->comp->end()) {
				if (it->hasWords()) {
					if (time < 0)
						time = it.key();
					else if (it.key() < time)
						time = it.key();
					break;
				}
			}
		}
	}
	return time;
}

void SubtitleRendererItem::setLoaded(const QList<LoadedSubtitle> &loaded) {
	unload();
	m_loaded = loaded;
	for (const auto &loaded : m_loaded) {
		if (loaded.isSelected())
			m_order.prepend(new Render(loaded.component()));
	}
	qSort(m_order.begin(), m_order.end(), [this] (const Render *lhs, const Render *rhs) {
		return d->language_priority(lhs) > d->language_priority(rhs);
	});
	applySelection();
}

QVector<SubtitleComponentModel*> SubtitleRendererItem::models() const {
	QVector<SubtitleComponentModel*> models;
	models.reserve(m_order.size());
	for (const auto render : m_order)
		models.push_back(render->model);
	return models;
}
