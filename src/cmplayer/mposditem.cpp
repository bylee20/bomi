#include "mposditem.hpp"

LetterboxItem::LetterboxItem(QQuickItem *parent)
: QQuickItem(parent) {
    setFlag(ItemHasContents, true);
}

bool LetterboxItem::set(const QRectF &outer, const QRectF &inner) {
    if (m_outer != outer || m_inner != inner) {
        m_outer = outer;
        m_inner = inner;
        m_screen = outer & inner;
        m_rectChanged = true;
        update();
        return true;
    } else
        return false;
}

QSGNode *LetterboxItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) {
    Q_UNUSED(data);
    QSGGeometryNode *node = static_cast<QSGGeometryNode*>(old);
    QSGGeometry *geometry = 0;

    if (!node) {
        node = new QSGGeometryNode;
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 16);
        geometry->setDrawingMode(GL_QUADS);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry);
        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(Qt::black);
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
        m_rectChanged = true;
    } else {
        geometry = node->geometry();
    }

    if (m_rectChanged) {
        auto vtx = geometry->vertexDataAsPoint2D();
            vtx->set(m_outer.left(), m_outer.top());
        (++vtx)->set(m_outer.right(), m_outer.top());
        (++vtx)->set(m_outer.right(), m_inner.top());
        (++vtx)->set(m_outer.left(), m_inner.top());

        (++vtx)->set(m_outer.left(), m_inner.bottom());
        (++vtx)->set(m_outer.right(), m_inner.bottom());
        (++vtx)->set(m_outer.right(), m_outer.bottom());
        (++vtx)->set(m_outer.left(), m_outer.bottom());

        (++vtx)->set(m_outer.left(), m_outer.top());
        (++vtx)->set(m_inner.left(), m_outer.top());
        (++vtx)->set(m_inner.left(), m_outer.bottom());
        (++vtx)->set(m_outer.left(), m_outer.bottom());

        (++vtx)->set(m_inner.right(), m_outer.top());
        (++vtx)->set(m_outer.right(), m_outer.top());
        (++vtx)->set(m_outer.right(), m_outer.bottom());
        (++vtx)->set(m_inner.right(), m_outer.bottom());

        m_rectChanged = false;
        node->markDirty(QSGNode::DirtyGeometry);
    }
    return node;
}

extern "C" {
#define new __new
#include <sub/sub.h>
#undef new
}

struct OsdBitmap {
	struct Part { QRect display = {0, 0, 0, 0};	int width = 0; QImage image; };
	QVector<Part> parts;
	int id = -1, pos = -1;
	sub_bitmap_format format = SUBBITMAP_EMPTY;
};

class MpOsdItemShader : public QSGMaterialShader {
public:
	MpOsdItemShader(MpOsdItem *item): m_item(item) {}
	void updateState(const RenderState &state, QSGMaterial *newOne, QSGMaterial *old) {
		Q_UNUSED(old); Q_UNUSED(newOne);
		auto prog = program();
		if (state.isMatrixDirty())
			prog->setUniformValue(m_loc_matrix, state.combinedMatrix());
		prog->setUniformValue(m_loc_tex_data, 0);
		prog->setUniformValue(m_loc_width, (float)m_item->frameSize().width());
		prog->setUniformValue(m_loc_height, (float)m_item->frameSize().height());
		m_item->updateState(prog);
	}
	void initialize() {
		QSGMaterialShader::initialize();
		m_loc_matrix = program()->uniformLocation("qt_Matrix");
		m_loc_tex_data = program()->uniformLocation("tex_data");
		m_loc_width = program()->uniformLocation("width");
		m_loc_height = program()->uniformLocation("height");
	}
private:
	char const *const *attributeNames() const;
	const char *vertexShader() const;
	const char *fragmentShader() const;
	int m_loc_matrix = 0, m_loc_tex_data = 0, m_loc_width = 0, m_loc_height = 0;
	MpOsdItem *m_item = nullptr;
};

char const *const *MpOsdItemShader::attributeNames() const {
	static const char *names[] = {
		"qt_VertexPosition",
		"qt_VertexTexCoord",
		0
	};
	return names;
}

const char *MpOsdItemShader::vertexShader() const {
	static const char *shader = (R"(
		uniform highp mat4 qt_Matrix;
		attribute highp vec4 qt_VertexPosition;
		attribute highp vec2 qt_VertexTexCoord;
		varying highp vec2 qt_TexCoord;
		void main() {
			qt_TexCoord = qt_VertexTexCoord;
			gl_Position = qt_Matrix * qt_VertexPosition;
		}
	)");
	return shader;
}

const char *MpOsdItemShader::fragmentShader() const {
	static const char *shader = (R"(
		uniform sampler2D tex_data;
		uniform float width, height;
		varying highp vec2 qt_TexCoord;
		void main() {
			vec2 size = vec2(width, height);
			vec3 dxy0 = vec3(1.0/width, 1.0/height, 0.0);
			ivec2 pixel = ivec2(qt_TexCoord*size);
			vec2 texel = (vec2(pixel)+vec2(0.5, 0.5))/size;

			vec4 x0y0 = texture2D(tex_data, texel);
			vec4 x1y0 = texture2D(tex_data, texel + dxy0.xz);
			vec4 x0y1 = texture2D(tex_data, texel + dxy0.zy);
			vec4 x1y1 = texture2D(tex_data, texel + dxy0.xy);

			float a = fract(qt_TexCoord.x*width);
			float b = fract(qt_TexCoord.y*height);
			gl_FragColor =  mix(mix(x0y0, x1y0, a), mix(x0y1, x1y1, a), b);
		}
	)");
	return shader;
}

static int MaterialId = 0;
static std::array<QSGMaterialType, 50> MaterialTypes;

struct MpOsdItemMaterial : public QSGMaterial {
	MpOsdItemMaterial(MpOsdItem *item): m_item(item) { setFlag(Blending); }
	QSGMaterialType *type() const { return &MaterialTypes[m_id]; }
	QSGMaterialShader *createShader() const { return new MpOsdItemShader(m_item); }
private:
	int m_id = ++MaterialId%MaterialTypes.size();
	MpOsdItem *m_item = nullptr;
};

struct MpOsdItemNode : public QSGGeometryNode {
	MpOsdItemNode(MpOsdItem *item, const QSize &frameSize): m_item(item) {
		setFlags(OwnsGeometry | OwnsMaterial);
		setMaterial(new MpOsdItemMaterial(m_item));
		setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4));
		markDirty(DirtyMaterial | DirtyGeometry);
		m_fbo = new QOpenGLFramebufferObject(frameSize, GL_TEXTURE_2D);
		m_device = new QOpenGLPaintDevice(frameSize);
		m_device->setPaintFlipped(true);
	}
	~MpOsdItemNode() {	delete m_fbo;}
	QOpenGLFramebufferObject *fbo() const {return m_fbo;}
	QOpenGLPaintDevice *device() const {return m_device;}
private:
	QOpenGLFramebufferObject *m_fbo = nullptr;
	QOpenGLPaintDevice *m_device = nullptr;
	MpOsdItem *m_item = nullptr;
};

struct MpOsdItem::Data {
	OsdBitmap osd;
	QMutex mutex;
	bool show = false, redraw = false, dirtyGeometry = true;
	sub_bitmap_format shaderFormat = SUBBITMAP_INDEXED;
	QOpenGLFramebufferObject *fbo = nullptr;
	QSize frameSize = {1, 1};
	MpOsdItemNode *node = nullptr;

	void paint(QPainter *painter) {
		for (const auto &part : osd.parts)
			painter->drawImage(part.display, part.image, QRect(0, 0, part.width, part.image.height()));
	}
};

MpOsdItem::MpOsdItem(QQuickItem *parent)
: QQuickItem(parent), d(new Data) {
	setFlag(ItemHasContents, true);
}

MpOsdItem::~MpOsdItem() {
	delete d;
}

QSize MpOsdItem::frameSize() const {
	return d->frameSize;
}

void MpOsdItem::setFrameSize(const QSize &size) {
	if (size != d->frameSize) {
		d->frameSize = size;
		update();
	}
}

void MpOsdItem::drawOn(sub_bitmaps *imgs) {
	d->show = true;
	if (imgs->num_parts <= 0 || imgs->format != SUBBITMAP_RGBA)
		return;
	if (d->osd.id == imgs->bitmap_id && d->osd.pos == imgs->bitmap_pos_id)
		return;
	d->mutex.lock();
	d->osd.id = imgs->bitmap_id;
	d->osd.pos = imgs->bitmap_pos_id;
	d->osd.parts.resize(imgs->num_parts);
	d->osd.format = imgs->format;
	for (int i=0; i<imgs->num_parts; ++i) {
		auto &img = imgs->parts[i];
		auto &part = d->osd.parts[i];
		part.image = QImage(img.stride >> 2, img.h, QImage::Format_ARGB32_Premultiplied);
		part.width = img.w;
		part.display = QRect(img.x, img.y, img.dw, img.dh);
		memcpy(part.image.bits(), img.bitmap, img.stride*img.h);
	}
	d->redraw = true;
	d->mutex.unlock();
}

void MpOsdItem::drawOn(QImage &frame) {
	if (!isVisible() || d->osd.format != SUBBITMAP_RGBA)
		return;
	d->mutex.lock();
	QPainter painter(&frame);
	d->paint(&painter);
	d->mutex.unlock();
}

void MpOsdItem::present() {
	if (d->show) {
		qApp->postEvent(this, new QEvent((QEvent::Type)(ShowEvent)));
		d->show = false;
	} else
		qApp->postEvent(this, new QEvent((QEvent::Type)(HideEvent)));
}

void MpOsdItem::customEvent(QEvent *event) {
	QQuickItem::customEvent(event);
	if (event->type() == ShowEvent) {
		setVisible(true);
		update();
	} else if (event->type() == HideEvent) {
		setVisible(false);
	}
}

QSGNode *MpOsdItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) {
	Q_UNUSED(data);
	d->node = static_cast<MpOsdItemNode*>(old);
	if (!d->node || d->node->fbo()->size() != d->frameSize) {
		delete d->node;
		d->node = new MpOsdItemNode(this, d->frameSize);
	}
	if (!old || d->redraw) {
		d->node->fbo()->bind();
		QPainter painter;
		painter.begin(d->node->device());
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.fillRect(QRect(QPoint(0, 0), d->frameSize), Qt::transparent);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		d->mutex.lock();
		d->paint(&painter);
		d->redraw = false;
		d->mutex.unlock();

		painter.end();
		d->node->fbo()->release();
	}
	if (!old || d->dirtyGeometry) {
		auto tp = d->node->geometry()->vertexDataAsTexturedPoint2D();
		const QRectF vtx = boundingRect();
		const QRectF txt(0, 0, 1, 1);
		auto set = [](QSGGeometry::TexturedPoint2D *tp, const QPointF &vtx, const QPointF &txt) {
			tp->set(vtx.x(), vtx.y(), txt.x(), txt.y());
		};
		set(tp, vtx.topLeft(), txt.topLeft());
		set(++tp, vtx.bottomLeft(), txt.bottomLeft());
		set(++tp, vtx.topRight(), txt.topRight());
		set(++tp, vtx.bottomRight(), txt.bottomRight());
		d->node->markDirty(QSGNode::DirtyGeometry);
	}
	return d->node;
}

void MpOsdItem::geometryChanged(const QRectF &newOne, const QRectF &old) {
	QQuickItem::geometryChanged(newOne, old);
	d->dirtyGeometry = true;
	update();
}

void MpOsdItem::updateState(QOpenGLShaderProgram */*program*/) {
	if (d->node && !d->frameSize.isEmpty()) {
		auto f = QOpenGLContext::currentContext()->functions();
		f->glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, d->node->fbo()->texture());
	}
}
