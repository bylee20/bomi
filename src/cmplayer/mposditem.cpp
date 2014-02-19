#include "mposditem.hpp"
#include "mposdbitmap.hpp"
#include "dataevent.hpp"
#include "openglcompat.hpp"

class MpOsdItemShaderProgram : public QOpenGLShaderProgram {
	enum {vPosition, vCoord, vColor};
public:
	static constexpr int N = 6;
	MpOsdItemShaderProgram(QObject *parent = nullptr)
		: QOpenGLShaderProgram(parent) { }
	void setFragmentShader(const QByteArray &code) {
		if (!m_frag)
			m_frag = addShaderFromSourceCode(QOpenGLShader::Fragment, code);
	}
	void setVertexShader(const QByteArray &code) {
		if (!m_vertex) {
			m_vertex = addShaderFromSourceCode(QOpenGLShader::Vertex, code);
			m_hasColor = code.contains("vColor");
		}
	}
	bool link() override {
		bindAttributeLocation("vCoord", vCoord);
		bindAttributeLocation("vPosition", vPosition);
		if (m_hasColor)
			bindAttributeLocation("vColor", vColor);
		return QOpenGLShaderProgram::link();
	}
	void setTextureCount(int textures) {
		if (_Expand(m_vPositions, 2*N*textures)) {
			m_vCoords.resize(m_vPositions.size());
			if (m_hasColor)
				m_vColors.resize(m_vPositions.size()/2);
		}
	}
	void uploadPositionAsTriangles(int i, const QPointF &p1, const QPointF &p2) {
		uploadRectAsTriangles(m_vPositions.data(), i, p1, p2);
	}
	void uploadPositionAsTriangles(int i, const QRectF &rect) {
		uploadRectAsTriangles(m_vPositions.data(), i, rect.topLeft(), rect.bottomRight());
	}
	void uploadCoordAsTriangles(int i, const QPointF &p1, const QPointF &p2) {
		uploadRectAsTriangles(m_vCoords.data(), i, p1, p2);
	}
	void uploadCoordAsTriangles(int i, const QRectF &rect) {
		uploadRectAsTriangles(m_vCoords.data(), i, rect.topLeft(), rect.bottomRight());
	}
	void uploadColorAsTriangles(int i, quint32 color) {
		auto p = m_vColors.data() + N*i;
		*p++ = color; *p++ = color; *p++ = color; *p++ = color; *p++ = color; *p++ = color;
	}
	void begin() {
		bind();
		enableAttributeArray(vPosition);
		enableAttributeArray(vCoord);
		setAttributeArray(vCoord, m_vCoords.data(), 2);
		setAttributeArray(vPosition, m_vPositions.data(), 2);
		if (m_hasColor) {
			enableAttributeArray(vColor);
			setAttributeArray(vColor, OGL::UInt8, m_vColors.data(), 4);
		}
	}
	void end() {
		disableAttributeArray(vCoord);
		disableAttributeArray(vPosition);
		if (m_hasColor)
			disableAttributeArray(vColor);
		release();
	}
	void reset() { removeAllShaders(); m_frag = m_vertex = false; }
private:
	void uploadRectAsTriangles(float *p, int i, const QPointF &p1, const QPointF &p2) {
		p += N*2*i;
		*p++ = p1.x(); *p++ = p1.y();
		*p++ = p2.x(); *p++ = p1.y();
		*p++ = p1.x(); *p++ = p2.y();

		*p++ = p1.x(); *p++ = p2.y();
		*p++ = p2.x(); *p++ = p2.y();
		*p++ = p2.x(); *p++ = p1.y();
	}
	QVector<float> m_vPositions, m_vCoords;
	QVector<quint32> m_vColors;
	bool m_hasColor = false, m_frag = false, m_vertex = false;
};


struct MpOsdItem::Data {
	MpOsdItem *p = nullptr;
	MpOsdBitmap osd;
	QSize imageSize = {0, 0}, atlasSize = {0, 0};
	bool show = false;
	MpOsdBitmap::Format format = MpOsdBitmap::Ass;
	GLenum srcFactor = GL_SRC_ALPHA;
	int loc_atlas = 0, loc_vMatrix = 0;
	MpOsdItemShaderProgram *shader = nullptr;
	OpenGLTexture2D atlas;
	OpenGLTextureTransferInfo textureTransfer;
	QMatrix4x4 vMatrix;

	void build(MpOsdBitmap::Format inFormat) {
		if (!_Change(format, inFormat) && shader)
			return;
		_Renew(shader);
		srcFactor = (format & MpOsdBitmap::PaMask) ? GL_ONE : GL_SRC_ALPHA;
		atlasSize = {};
		textureTransfer = OpenGLCompat::textureTransferInfo(format & MpOsdBitmap::Rgba ? OGL::BGRA : OGL::OneComponent);

		QByteArray frag;
		if (format == MpOsdBitmap::Ass) {
			frag = R"(
				uniform sampler2D atlas;
				varying vec4 c;
				varying vec2 texCoord;
				void main() {
					vec2 co = vec2(c.a*texture2D(atlas, texCoord).r, 0.0);
					gl_FragColor = c*co.xxxy + co.yyyx;
				}
			)";
		} else {
			frag = R"(
				uniform sampler2D atlas;
				varying vec2 texCoord;
				void main() {
					gl_FragColor = texture2D(atlas, texCoord);
				}
			)";
		}
		shader->setFragmentShader(frag);
		shader->setVertexShader(R"(
			uniform mat4 vMatrix;
			varying vec4 c;
			varying vec2 texCoord;
			attribute vec4 vPosition;
			attribute vec2 vCoord;
			attribute vec4 vColor;
			void main() {
				c = vColor.abgr;
				texCoord = vCoord;
				gl_Position = vMatrix*vPosition;
			}
		)");
		shader->link();
		loc_atlas = shader->uniformLocation("atlas");
		loc_vMatrix = shader->uniformLocation("vMatrix");
	}

	void upload(const MpOsdBitmap &osd, int i) {
		auto &part = osd.part(i);
		atlas.upload(part.map().x(), part.map().y(), part.strideAsPixel(), part.height(), osd.data(i));
		QPointF tp = part.map(); tp.rx() /= (double)atlas.width(); tp.ry() /= (double)atlas.height();
		QSizeF ts = part.size(); ts.rwidth() /= (double)atlas.width(); ts.rheight() /= (double)atlas.height();
		shader->uploadCoordAsTriangles(i, {tp, ts});
		shader->uploadPositionAsTriangles(i, part.display());
		shader->uploadColorAsTriangles(i, part.color());
	}

	void initializeAtlas(const MpOsdBitmap &osd) {
		static const int max = OpenGLCompat::maximumTextureSize();
		if (osd.sheet().width() > atlasSize.width() || osd.sheet().height() > atlasSize.height()) {
			if (osd.sheet().width() > atlasSize.width())
				atlasSize.rwidth() = qMin<int>(_Aligned<4>(osd.sheet().width()*1.5), max);
			if (osd.sheet().height() > atlasSize.height())
				atlasSize.rheight() = qMin<int>(_Aligned<4>(osd.sheet().height()*1.5), max);
			glEnable(atlas.target());
			if (atlas.id() == GL_NONE)
				atlas.create(OGL::Linear, OGL::ClampToEdge);
			OpenGLTextureBinder<OGL::Target2D> binder(&atlas);
			atlas.initialize(atlasSize, textureTransfer);
		}
	}

	void draw(OpenGLFramebufferObject *fbo, const MpOsdBitmap &osd) {
        if (!fbo->isValid())
			return;
		build(osd.format());
		if (!shader->isLinked())
			return;
		initializeAtlas(osd);

		Q_ASSERT(fbo->size() == osd.renderSize());
		vMatrix.setToIdentity();
		vMatrix.ortho(0, fbo->width(), 0, fbo->height(), -1, 1);

		fbo->bind();
		glViewport(0, 0, fbo->width(), fbo->height());
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		OpenGLTextureBinder<OGL::Target2D> binder(&atlas);

		shader->setTextureCount(osd.count());
		for (int i=0; i<osd.count(); ++i)
			upload(osd, i);

		shader->begin();
		shader->setUniformValue(loc_atlas, 0);
		shader->setUniformValue(loc_vMatrix, vMatrix);

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_BLEND);
		glBlendFunc(srcFactor, GL_ONE_MINUS_SRC_ALPHA);
		glDrawArrays(GL_TRIANGLES, 0, shader->N*osd.count());
		glDisable(GL_BLEND);

		shader->end();
		fbo->release();
	}
};

MpOsdItem::MpOsdItem(QQuickItem *parent)
: FramebufferObjectRendererItem(parent), d(new Data) {
	d->p = this;
}

MpOsdItem::~MpOsdItem() {
	delete d;
}

void MpOsdItem::initializeGL() {
	FramebufferObjectRendererItem::initializeGL();
}

void MpOsdItem::finalizeGL() {
	FramebufferObjectRendererItem::finalizeGL();
	_Delete(d->shader);
}

void MpOsdItem::drawOn(sub_bitmaps *imgs) {
	d->show = true;
	MpOsdBitmap osd;
	if (osd.copy(imgs, d->imageSize))
		_PostEvent(this, EnqueueFrame, osd);
}

void MpOsdItem::drawOn(QImage &frame) {
	if (isVisible())
		d->osd.drawOn(frame);
}

void MpOsdItem::present(bool redraw) {
	if (redraw)
		return;
	if (d->show) {
		_PostEvent(this, Show);
		d->show = false;
	} else
		_PostEvent(this, Hide);
}

void MpOsdItem::customEvent(QEvent *event) {
	QQuickItem::customEvent(event);
	switch ((int)event->type()) {
	case Show:
		setVisible(true);
		update();
		break;
	case Hide:
		setVisible(false);
		break;
	case EnqueueFrame:
		setVisible(true);
		_GetAllData(event, d->osd);
		forceRepaint();
		break;
	default:
		break;
	}
}

QSize MpOsdItem::imageSize() const {
	return d->osd.renderSize();
}

void MpOsdItem::paint(OpenGLFramebufferObject *fbo) {
	d->draw(fbo, d->osd);
}

void MpOsdItem::setImageSize(const QSize &size) {
	d->imageSize = size;
}
