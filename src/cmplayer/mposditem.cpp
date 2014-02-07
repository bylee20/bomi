#include "mposditem.hpp"
#include "mposdbitmap.hpp"
#include "dataevent.hpp"
#include "openglcompat.hpp"

struct MpOsdItem::Data {
	MpOsdItem *p = nullptr;
	MpOsdBitmap osd;
	QSize imageSize = {0, 0};
	bool show = false;
	MpOsdBitmap::Format format = MpOsdBitmap::Ass;
	GLenum srcFactor = GL_SRC_ALPHA;
	int loc_atlas = 0, loc_vMatrix = 0;
	OpenGLTextureShaderProgram *shader = nullptr;
	OpenGLTexture atlas;
	QMatrix4x4 vMatrix;

	void build(MpOsdBitmap::Format inFormat) {
		if (!_Change(format, inFormat))
			return;
		_Renew(shader);
		srcFactor = (format & MpOsdBitmap::PaMask) ? GL_ONE : GL_SRC_ALPHA;
		atlas.width = atlas.height = 0;
		atlas.format = OpenGLCompat::textureFormat(format & MpOsdBitmap::Rgba ? GL_BGRA : 1);

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
		QPointF tp = part.map(); tp.rx() /= (double)atlas.width; tp.ry() /= (double)atlas.height;
		QSizeF ts = part.size(); ts.rwidth() /= (double)atlas.width; ts.rheight() /= (double)atlas.height;
		shader->uploadCoordAsTriangles(i, {tp, ts});
		shader->uploadPositionAsTriangles(i, part.display());
		shader->uploadColorAsTriangles(i, part.color());
	}

	void initializeAtlas(const MpOsdBitmap &osd) {
		static const int max = OpenGLCompat::maximumTextureSize();
		if (osd.sheet().width() > atlas.width || osd.sheet().height() > atlas.height) {
			if (osd.sheet().width() > atlas.width)
				atlas.width = qMin<int>(_Aligned<4>(osd.sheet().width()*1.5), max);
			if (osd.sheet().height() > atlas.height)
				atlas.height = qMin<int>(_Aligned<4>(osd.sheet().height()*1.5), max);
			glEnable(atlas.target);
			if (atlas.id == GL_NONE)
				atlas.generate();
			atlas.allocate(GL_NEAREST, GL_CLAMP_TO_EDGE);
		}
	}

	void draw(OpenGLFramebufferObject *fbo, const MpOsdBitmap &osd) {
		if (fbo->isNull())
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

		shader->setTextureCount(osd.count());
		for (int i=0; i<osd.count(); ++i)
			upload(osd, i);

		shader->begin();
		shader->setUniformValue(loc_atlas, 0);
		shader->setUniformValue(loc_vMatrix, vMatrix);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlas.id);
		glEnable(GL_BLEND);
		glBlendFunc(srcFactor, GL_ONE_MINUS_SRC_ALPHA);
		glDrawArrays(GL_TRIANGLES, 0, shader->N*osd.count());
		glDisable(GL_BLEND);
		glBindTexture(GL_TEXTURE_2D, 0);

		shader->end();
		fbo->release();
		LOG_GL_ERROR
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
