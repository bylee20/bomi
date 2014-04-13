#include "subtitlerendereritem.hpp"
#include "dataevent.hpp"
#include "subtitlerenderingthread.hpp"
#include "openglcompat.hpp"

struct SubtitleRendererItem::Data {
	Data(SubtitleRendererItem *p): p(p) {}
	SubtitleRendererItem *p = nullptr;
	QList<SubComp*> loaded;
	QSize imageSize{0, 0};
	SubtitleDrawer drawer;
	int delay = 0, msec = 0;
	bool selecting = false, redraw = false, textChanged = true;
	bool top = false, hidden = false, empty = true;
	double pos = 1.0;
	QMap<QString, int> langMap;
	QMutex mutex;
	QWaitCondition wait;
	RichTextDocument text;
	int language_priority(const SubComp &comp) const {
//		return langMap.value(r->comp->language().id(), -1);
		return langMap.value(comp.language(), -1);
	}
	QVector<quint32> zeros, bboxData;
	SubCompSelection selection{p};
	OpenGLTexture2D texture, bbox;
	double fps() const { return selection.fps(); }
	void updateDrawer() {
		selection.setDrawer(drawer);
		p->setGeometryDirty();
	}
	void sort() {
		selection.sort([this] (const SubComp &lhs, const SubComp &rhs) {
			return language_priority(lhs) > language_priority(rhs);
		});
	}
	void setMargin(const Margin &margin) {
		drawer.setMargin(margin);
		p->setGeometryDirty();
		p->update();
	}
	void applySelection() {
		empty = selection.isEmpty();
		emit p->modelsChanged(selection.models());
		if (!empty)
			render(SubCompSelection::Rerender);
		else
			p->setVisible(false);
		textChanged = true;
	}
	void render(int flags) {
		if (!hidden && !empty && msec > 0)
			selection.render(msec - delay, flags);
	}
	void updateVisible() {
		p->setVisible(!hidden && !empty && !imageSize.isEmpty());
	}
};

class SubtitleRendererShader : public TextureRendererShader {
public:
	SubtitleRendererShader(const SubtitleRendererItem *item)
	: TextureRendererShader(item) { }
	const char *fragmentShader() const {
		const char *shader = (R"(
			uniform sampler2D tex;
			uniform sampler2D bbox;
			uniform vec4 bboxColor;
			varying vec2 texCoord;
			void main() {
				vec4 top = texture2D(tex, texCoord);
				float alpha = texture2D(bbox, texCoord).a*bboxColor.a*(1.0 - top.a);
				gl_FragColor = top + bboxColor*alpha;
			}
		)");
		return shader;
	}
	void link(QOpenGLShaderProgram *prog) override {
		loc_tex = prog->uniformLocation("tex");
		loc_bbox = prog->uniformLocation("bbox");
		loc_bboxColor = prog->uniformLocation("bboxColor");
	}
	void bind(QOpenGLShaderProgram *prog) override {
		auto d = static_cast<const SubtitleRendererItem*>(item())->d;
		prog->setUniformValue(loc_tex, 0);
		prog->setUniformValue(loc_bbox, 1);
		prog->setUniformValue(loc_bboxColor, d->drawer.style().bbox.color);
		func()->glActiveTexture(GL_TEXTURE1);
		d->bbox.bind();
	}
private:
	int loc_tex = -1, loc_bbox = -1, loc_bboxColor = -1;
};

SubtitleRendererItem::SubtitleRendererItem(QQuickItem *parent)
	: HQTextureRendererItem(parent), d(new Data(this)) {
	d->drawer.setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
	d->updateDrawer();
}

SubtitleRendererItem::~SubtitleRendererItem() {
	unload();
	delete d;
}

bool SubtitleRendererItem::isTopAligned() const {
	return d->top;
}

void SubtitleRendererItem::render(int ms) {
	d->msec = ms;
	d->render(SubCompSelection::Tick);
}

void SubtitleRendererItem::rerender() {
	d->render(SubCompSelection::Rerender);
}

TextureRendererShader *SubtitleRendererItem::createShader() const {
	return new SubtitleRendererShader(this);
}

void SubtitleRendererItem::initializeGL() {
	HQTextureRendererItem::initializeGL();
	d->texture.create();
	d->bbox.create();
	setRenderTarget(d->texture);
}

void SubtitleRendererItem::finalizeGL() {
	HQTextureRendererItem::finalizeGL();
	d->bbox.destroy();
	d->texture.destroy();
}

const RichTextDocument &SubtitleRendererItem::text() const {
	if (d->textChanged) {
		d->text.clear();
		d->selection.forImages([this] (const SubCompImage &image) {
			d->text += image.text();
		});
		d->textChanged = false;
	}
	return d->text;
}

void SubtitleRendererItem::setPriority(const QStringList &priority) {
	for (int i=0; i< priority.size(); ++i)
		d->langMap[priority[i]] = priority.size()-i;
}

void SubtitleRendererItem::setStyle(const SubtitleStyle &style) {
	d->drawer.setStyle(style);
	d->updateDrawer();
}

static inline QRectF operator * (const QRectF &rect, double p) {
	return {rect.topLeft()*p, rect.size()*p};
}

static inline QRectF operator / (const QRectF &rect, double p) {
	return {rect.topLeft()/p, rect.size()/p};
}

QImage SubtitleRendererItem::draw(const QRectF &rect, QRectF *put) const {
	QImage sub; int gap = 0;
	auto boxes = d->drawer.draw(sub, gap, text(), rect, 1.0);
	if (sub.isNull())
		return QImage();
	if (put)
		*put = {d->drawer.pos(sub.size(), rect), sub.size()};
	if (!boxes.isEmpty()) {
		QImage bg(sub.size(), QImage::Format_ARGB32_Premultiplied);
		bg.fill(0x0);
		QPainter painter(&bg);
		auto bcolor = d->drawer.style().bbox.color;
		bcolor.setAlpha(255);
		for (auto &bbox : boxes)
			painter.fillRect(bbox, bcolor);
		auto p = bg.bits();
		for (int i=0; i<bg.width(); ++i) {
			for (int j=0; j<bg.height(); ++j) {
				*p++ *= d->drawer.style().bbox.color.alphaF();
				*p++ *= d->drawer.style().bbox.color.alphaF();
				*p++ *= d->drawer.style().bbox.color.alphaF();
				*p++ *= d->drawer.style().bbox.color.alphaF();
			}
		}
		painter.drawImage(QPoint(0, 0), sub);
		painter.end();
		sub.swap(bg);
	}
	return sub;
}

double SubtitleRendererItem::pos() const {
	return d->pos;
}

int SubtitleRendererItem::componentsCount() const {
	return d->loaded.count();
}

QList<const SubComp *> SubtitleRendererItem::components() const {
	QList<const SubComp*> list; list.reserve(d->loaded.size());
	for (auto loaded : d->loaded)
		list << loaded;
	return list;
}

void SubtitleRendererItem::setTopAligned(bool top) {
	if (_Change(d->top, top)) {
		const auto alignment = Qt::AlignHCenter | (d->top ? Qt::AlignTop : Qt::AlignBottom);
		if (d->drawer.alignment() != alignment) {
			d->drawer.setAlignment(alignment);
			d->updateDrawer();
		}
		setPos(1.0 - d->pos);
	}
}

const SubtitleStyle &SubtitleRendererItem::style() const {
	return d->drawer.style();
}

void SubtitleRendererItem::unload() {
	d->selection.clear();
	qDeleteAll(d->loaded);
	d->loaded.clear();
	setVisible(false);
	d->empty = true;
	d->textChanged = true;
	emit modelsChanged(models());
}

void SubtitleRendererItem::getCoords(QRectF &vertices, QRectF &) {
	const auto dpr = devicePixelRatio();
	vertices = QRectF(d->drawer. pos(d->imageSize/dpr, rect()), d->imageSize/dpr);
}

void SubtitleRendererItem::prepare(QSGGeometryNode *node) {
	if (d->redraw) {
		d->imageSize = {0, 0};
		d->selection.forImages([this] (const SubCompImage &image) {
			if (d->imageSize.width() < image.width())
				d->imageSize.rwidth() = image.width();
			d->imageSize.rheight() += image.height();
		});
		if (!d->imageSize.isEmpty()) {
			const auto len = d->imageSize.width()*d->imageSize.height();
			_Expand(d->zeros, len);
			OpenGLTextureBinder<OGL::Target2D> binder;
			binder.bind(&d->texture);
			d->texture.initialize(d->imageSize, d->zeros.data());
			binder.bind(&d->bbox);
			d->bbox.initialize(d->imageSize, d->zeros.data());
			int y = 0;
			d->selection.forImages([this, &y, &binder] (const SubCompImage &image) {
				const int x = (d->texture.width() - image.width())*0.5;
				if (!image.isNull()) {
					binder.bind(&d->texture);
					d->texture.upload(x, y, image.width(), image.height(), image.bits());
					binder.bind(&d->bbox);
					for (auto &bbox : image.boundingBoxes()) {
						const auto rect = bbox.toRect().translated(x, y);
						if (_Expand(d->bboxData, rect.width()*rect.height()))
							d->bboxData.fill(_Max<quint32>());
						d->bbox.upload(rect, d->bboxData.data());
					}
				}
				y += image.height();
			});
			setGeometryDirty();
			node->markDirty(QSGNode::DirtyMaterial);
		}
		d->redraw = false;
	}
	d->updateVisible();
	setVisible(!d->hidden && !d->empty && !d->imageSize.isEmpty());
}

bool SubtitleRendererItem::isHidden() const {
	return d->hidden;
}

void SubtitleRendererItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
	d->selection.setArea(rect(), devicePixelRatio());
	HQTextureRendererItem::geometryChanged(newGeometry, oldGeometry);
}

int SubtitleRendererItem::delay() const {
	return d->delay;
}

void SubtitleRendererItem::setDelay(int delay) {
	if (_Change(d->delay, delay))
		rerender();
}

void SubtitleRendererItem::setPos(double pos) {
	if (_ChangeF(d->pos, qBound(0.0, pos, 1.0)))
		d->setMargin({{0.0, d->top ? d->pos : 0}, {0.0, d->top ? 0.0 : 1.0 - d->pos}});
}

void SubtitleRendererItem::setHidden(bool hidden) {
	if (_Change(d->hidden, hidden))
		d->updateVisible();
}

void SubtitleRendererItem::setFPS(double fps) {
	d->selection.setFPS(fps);
}

double SubtitleRendererItem::fps() const {
	return d->fps();
}

void SubtitleRendererItem::deselect(int idx) {
	if (idx < 0)
		d->selection.clear();
	else if (_InRange(0, idx, d->loaded.size()-1) && d->loaded[idx]->selection())
		d->selection.remove(d->loaded[idx]);
	else
		return;
	if (!d->selecting)
		d->applySelection();
}

void SubtitleRendererItem::select(int idx) {
	bool sort = false;
	if (idx < 0) {
		for (auto comp : d->loaded) {
			if (d->selection.prepend(comp))
				sort = true;
		}
	} else if (_InRange(0, idx, d->loaded.size()-1) && !d->loaded[idx]->selection())
		sort = d->selection.prepend(d->loaded[idx]);
	if (sort) {
		d->sort();
		if (!d->selecting)
			d->applySelection();
	}
}

bool SubtitleRendererItem::load(const Subtitle &subtitle, bool select) {
	if (subtitle.isEmpty())
		return false;
	const int idx = d->loaded.size();
	for (int i=0; i<subtitle.size(); ++i)
		d->loaded.append(new SubComp(subtitle[i]));
	if (select) {
		d->selecting = true;
		for (int i=d->loaded.size()-1; i>=idx; --i) {
			if (select)
				this->select(i);
		}
		d->selecting = false;
		d->applySelection();
	}
	return true;
}

int SubtitleRendererItem::start(int time) const {
	int ret = -1;
	d->selection.forComponents([this, time, &ret] (const SubComp &comp) {
		const auto it = comp.start(time - d->delay, d->fps());
		if (it != comp.end())
			ret = qMax(ret, comp.isBasedOnFrame() ? comp.msec(it.key(), d->fps()) : it.key());
	});
	return ret;
}

int SubtitleRendererItem::finish(int time) const {
	int ret = -1;
	d->selection.forComponents([this, time, &ret] (const SubComp &comp) {
		const auto it = comp.finish(time - d->delay, d->fps());
		if (it != comp.end()) {
			const int t = comp.isBasedOnFrame() ? comp.msec(it.key(), d->fps()) : it.key();
			ret = ret == -1 ? t : qMin(ret, t);
		}
	});
	return ret;
}

static bool updateIfEarlier(SubComp::ConstIt it, int &time) {
	if (it->hasWords()) {
		if (time < 0)
			time = it.key();
		else if (it.key() > time)
			time = it.key();
		return true;
	} else
		return false;
}

int SubtitleRendererItem::current() const {
	int time = -1;
	d->selection.forImages([this, &time] (const SubCompImage &imageture) {
		if (imageture.isValid())
			updateIfEarlier(imageture.iterator(), time);
	});
	return time;
}

int SubtitleRendererItem::previous() const {
	int time = -1;
	d->selection.forImages([this, &time] (const SubCompImage &imageture) {
		if (imageture.isValid()) {
			auto it = imageture.iterator();
			while (it != imageture.component()->begin()) {
				if (updateIfEarlier(--it, time))
					break;
			}
		}
	});
	return time;
}

int SubtitleRendererItem::next() const {
	int time = -1;
	d->selection.forImages([this, &time] (const SubCompImage &imageture) {
		if (imageture.isValid()) {
			auto it = imageture.iterator();
			while (++it != imageture.component()->end()) {
				if (updateIfEarlier(it, time))
					break;
			}
		}
	});
	return time;
}

void SubtitleRendererItem::setComponents(const QList<SubComp> &components) {
	unload();
	d->loaded.reserve(components.size());
	for (const auto &comp : components)
		d->loaded.push_back(new SubComp(comp));
	for (const auto &comp : d->loaded) {
		if (comp->selection())
			d->selection.prepend(comp);
	}
	d->sort();
	d->applySelection();
}

QVector<SubCompModel*> SubtitleRendererItem::models() const {
	return d->selection.models();
}

void SubtitleRendererItem::customEvent(QEvent *event) {
	if (event->type() == SubCompSelection::ImagePrepared) {
		if (d->selection.update(_GetData<SubCompImage>(event)))
			d->textChanged = true;
		d->redraw = true;
		update();
	}
}
