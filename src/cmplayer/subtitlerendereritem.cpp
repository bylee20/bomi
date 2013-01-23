#include "subtitlerendereritem.hpp"
#include "richtextdocument.hpp"
#include "subtitlemodel.hpp"

struct SubtitleRender {
	SubtitleRender() {comp = 0; model = 0;}
	SubtitleRender(const SubtitleComponent &comp): comp(&comp) {
		model = new SubtitleComponentModel(this->comp);
		prev = this->comp->end();
	}
	~SubtitleRender() { delete this->model; }

	const SubtitleComponent *comp;
	SubtitleComponent::const_iterator prev;
	SubtitleComponentModel *model;
};

typedef QList<SubtitleRender*> RenderList;

struct SubtitleRendererItem::Data {
	bool docempty = true;
	RichTextDocument front, back;
	double top = 0.0, bottom = 0.0, left = 0.0, right = 0.0;
	bool redraw = false;
	int loc_tex = 0, loc_shadowColor = 0, loc_shadowOffset = 0, loc_dxdy = 0;
	QImage image, next, blank = {1, 1, QImage::Format_ARGB32_Premultiplied};
	QRectF screen = {0, 0, 0, 0};
	QPointF shadowOffset = {0, 0};
	QSize textureSize = {0, 0};
	QSize imageSize = {0, 0};


	bool	selecting = false;
	SubtitleRendererItem *item = nullptr;
	RenderList order;

	void reset_prev() {
		RenderList::const_iterator it = order.begin();
		for (; it!=order.end(); ++it) {
			SubtitleRender *r = *it;
			r->prev = r->comp->end();
		}
	}

	QMap<QString, int> langMap;
	int language_priority(const SubtitleRender *r) const {
//		return langMap.value(r->comp->language().id(), -1);
		return langMap.value(r->comp->language(), -1);
	}

	bool hasComponent() const {
		for (auto render : order) {
			if (!render->comp->isEmpty())
				return true;
		}
		return false;
	}
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
	d->docempty = !d->front.hasWords();
	if (!d->docempty) {
		double scale = this->scale();
		d->front.updateLayoutInfo();
		d->back.updateLayoutInfo();
		d->front.doLayout(width()/scale);
		d->back.doLayout(width()/scale);

		d->shadowOffset = m_style->shadow()->offset()*m_style->font()->height()*scale;
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
	program->setUniformValue(d->loc_shadowColor, m_style->shadow()->color());
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

void SubtitleRendererItem::setLetterboxHint(bool hint) {
	if (m_letterbox != hint) {
		m_letterbox = hint;
		if (d->screen != boundingRect()) {
			prepare();
			update();
		}
	}
}
















//struct SubtitleRendererItem::Data {
//	SubtitleView *view = nullptr;

//	double	fps = 25.0,			pos = 1.0;
//	int		delay = 0,			ms = 0;
//	bool	visible = true,		empty = true;
//	bool	selecting = false,	top = false;
//	SubtitleRendererItem *item = nullptr;
//	QList<Loaded> loaded;
//	RenderList order;




////	void reset_lang_map() {
////		const QStringList priority = cPref.sub_priority;
////		for (int i=0; i< priority.size(); ++i)
////			langMap[priority[i]] = priority.size()-i;
////	}
//};

//QWidget *SubtitleRendererItem::view(QWidget *parent) const {
//	if (!d->view) {
//		d->view = new SubtitleView(parent);
//		d->set_model_list();
//	}
//	return d->view;
//}

void SubtitleRendererItem::setFps(double fps) {
	if (m_fps != fps) {
		m_fps = fps;
		d->reset_prev();
	}
}

void SubtitleRendererItem::render(int ms) {
	m_ms = ms;
	if (!m_visible || m_compempty || ms == 0)
		return;
	bool changed = false;
	for (SubtitleRender *render : d->order) {
		auto it = render->comp->start(ms - m_delay, m_fps);
		if (it != render->prev) {
			render->prev = it;
			render->model->setCurrentCaption(&(*it));
			changed = true;
		}
	}
	if (changed) {
		RichTextDocument doc;
		for (auto o = d->order.begin(); o != d->order.end(); ++o) {
			const SubtitleRender &render = **o;
			if (render.prev != render.comp->end())
				doc += render.prev.value();
		}
		if (d->item) {
			d->item->setText(doc);
		}
	}
}

void SubtitleRendererItem::clear() {
	d->reset_prev();
//	d->osd.clear();
}

void SubtitleRendererItem::unload() {
	qDeleteAll(d->order);
	d->order.clear();
	m_loaded.clear();
//	d->set_model_list();
	m_compempty = true;
	clear();
}

void SubtitleRendererItem::select(int idx, bool selected) {
	if (0 <= idx && idx < m_loaded.size() && m_loaded[idx].isSelected() != selected) {
		m_loaded[idx].selection() = selected;
		RenderList::iterator o = d->order.begin();
		const auto comp = &m_loaded[idx].component();
		SubtitleRender *render = 0;
		for (; o!= d->order.end(); ++o) {
			if ((*o)->comp == comp) {
				render = *o;
				d->order.erase(o);
				render->prev = comp->end();
				break;
			}
		}
		if (selected) {
			if (!render)
				render = new SubtitleRender(*comp);
			d->order.prepend(render);
			o = d->order.begin();
			while (o != d->order.end()) {
				SubtitleRender *&prev = *o;
				if (++o == d->order.end())
					break;
				SubtitleRender *&next = *o;
				if (d->language_priority(prev) >= d->language_priority(next))
					break;
				qSwap(prev, next);
			}
		}
		if (!d->selecting)
			applySelection();
	}
}

void SubtitleRendererItem::applySelection() {
	m_compempty = !d->hasComponent();
	rerender();
}

bool SubtitleRendererItem::load(const QString &fileName, const QString &enc, bool select) {
	const int idx = m_loaded.size();
	Subtitle sub;
	if (!sub.load(fileName, enc))
		return false;
	for (int i=0; i<sub.size(); ++i) {
		m_loaded.append(LoadedSubtitle(sub[i]));
	}
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
	int s = -1;
	RenderList::const_iterator it = d->order.begin();
	for (; it != d->order.end(); ++it) {
		const auto comp = (*it)->comp;
		const auto it = comp->start(time - m_delay, m_fps);
		if (it != comp->end())
			s = qMax(s, comp->isBasedOnFrame() ? SubtitleComponent::msec(it.key(), m_fps) : it.key());
	}
	return s;
}

int SubtitleRendererItem::end(int time) const {
	int e = -1;
	RenderList::const_iterator it = d->order.begin();
	for (; it != d->order.end(); ++it) {
		const auto comp = (*it)->comp;
		const auto it = comp->finish(time - m_delay, m_fps);
		if (it != comp->end()) {
			const int t = comp->isBasedOnFrame() ? SubtitleComponent::msec(it.key(), m_fps) : it.key();
			e = e == -1 ? t : qMin(e, t);
		}
	}
	return e;
}

void SubtitleRendererItem::setTopAlignment(bool top) {
	if (_Change(m_alignTop, top)) {
		setAlignment(Qt::AlignHCenter | (m_alignTop ? Qt::AlignTop : Qt::AlignBottom));
		setPos(1.0 - m_pos);
	}
}

int SubtitleRendererItem::current() const {
	RenderList::const_iterator o = d->order.begin();
	int time = -1;
	for (; o != d->order.end(); ++o) {
		SubtitleRender &render = **o;
		if (render.prev == render.comp->end() || !render.prev->hasWords())
			continue;
		if (time < 0)
			time = render.prev.key();
		else if (render.prev.key() > time)
			time = render.prev.key();
	}
	return time;
}

int SubtitleRendererItem::previous() const {
	RenderList::const_iterator o = d->order.begin();
	int time = -1;
	QList<SubtitleComponent::const_iterator> its;
	its.reserve(d->order.size());
	for (; o != d->order.end(); ++o) {
		SubtitleRender &r = **o;
		if (r.prev == r.comp->end())
			continue;
		auto it = r.prev;
		while (it != r.comp->begin()) {
			if ((--it)->hasWords()) {
				if (time < 0)
					time = it.key();
				else if (it.key() > time)
					time = it.key();
				break;
			}
		}
	}
	return time;
}

int SubtitleRendererItem::next() const {
	RenderList::const_iterator o = d->order.begin();
	int time = -1;
	QList<SubtitleComponent::const_iterator> its;
	its.reserve(d->order.size());
	for (; o != d->order.end(); ++o) {
		SubtitleRender &r = **o;
		if (r.prev == r.comp->end())
			continue;
		auto it = r.prev;
		while (++it != r.comp->end()) {
			if (it->hasWords()) {
				if (time < 0)
					time = it.key();
				else if (it.key() < time)
					time = it.key();
				break;
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
			d->order.prepend(new SubtitleRender(loaded.component()));
	}
	qSort(d->order.begin(), d->order.end(), [this] (const SubtitleRender *lhs, const SubtitleRender *rhs) {
		return d->language_priority(lhs) > d->language_priority(rhs);
	});
	applySelection();
}

