#include "mposditem.hpp"

struct OsdData {
	int x = 0, y = 0, w = 0, h = 0, stride = 0;
	uchar *src = nullptr, *srca = nullptr;
};

struct MpOsdItem::Data {
	int loc_tex_y = 0, loc_tex_a = 0, stride = 0;
	QByteArray luma, alpha;
	bool empty = true, redraw = true;
	QRectF vertextRect = {.0, .0, .0, .0}, textureRect = {.0, .0, .0, .0};
	QRect osdRect = {0, 0, 0, 0};
	OsdData data;
	QMutex mutex;
	QSize frameSize = {1, 1};
	bool frameChanged = false;
};

MpOsdItem::MpOsdItem(QQuickItem *parent)
: TextureRendererItem(2, parent), d(new Data) {
	setVisible(false);
}

MpOsdItem::~MpOsdItem() {
	delete d;
}

void MpOsdItem::setFrameSize(const QSize &size) {
	d->frameSize = size;
	update();
}

void MpOsdItem::prepare() {
	setVisible(!d->empty);
}

void MpOsdItem::customEvent(QEvent *event) {
	TextureRendererItem::customEvent(event);
	if (event->type() == DrawnEvent) {
		prepare();
		update();
	}
}

void MpOsdItem::link(QOpenGLShaderProgram *program) {
	TextureRendererItem::link(program);
	d->loc_tex_a = program->uniformLocation("tex_y");
	d->loc_tex_y = program->uniformLocation("tex_a");
}

void MpOsdItem::bind(const RenderState &state, QOpenGLShaderProgram *program) {
	TextureRendererItem::bind(state, program);
	program->setUniformValue(d->loc_tex_y, 0);
	program->setUniformValue(d->loc_tex_a, 1);
	if (!d->empty) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture(0));
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture(1));
		glActiveTexture(GL_TEXTURE0);
	}
}

void MpOsdItem::updateTexturedPoint2D(TexturedPoint2D *tp) {
	const auto exp_x = width()/d->frameSize.width();
	const auto exp_y = height()/d->frameSize.height();
	const QPointF pos(d->osdRect.x()*exp_x, d->osdRect.y()*exp_y);
	const QSizeF size(d->osdRect.width()*exp_x, d->osdRect.height()*exp_y);
	set(tp, QRectF(pos, size), d->textureRect);
}

void MpOsdItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
	TextureRendererItem::geometryChanged(newGeometry, oldGeometry);
	setGeometryDirty();
	prepare();
	update();
}

void MpOsdItem::setFrameChanged() {
	d->data.x = d->data.y = d->data.w = d->data.h = d->data.stride = 0;
}

QMutex &MpOsdItem::mutex() const {
	return d->mutex;
}

void MpOsdItem::draw(int x, int y, int w, int h, uchar *src, uchar *srca, int stride) {
	const int len = h*stride;
	if ((d->empty = len <= 0))
		return;
	QMutexLocker locker(&d->mutex);
	if (d->data.x == x && d->data.y == y && d->data.w == w && d->data.h == h && d->data.src == src && d->data.srca == srca && d->data.stride == stride)
		return;
	d->data.x = x;
	d->data.y = y;
	d->data.w = w;
	d->data.h = h;
	d->data.src = src;
	d->data.srca = srca;
	d->data.stride = stride;

	d->stride = stride;
	d->osdRect = QRect(x, y, w, h);
	d->luma.resize(len);
	fast_memcpy(d->luma.data(), src, len);
	d->alpha.resize(len);
	char *alpha = d->alpha.data();
	for (int i=0; i<len; ++i)
		*alpha++ = -*srca++;
	d->textureRect.setWidth((double)w/(double)stride);
	d->redraw = true;
	qApp->postEvent(this, new QEvent((QEvent::Type)DrawnEvent));
}


void MpOsdItem::beforeUpdate() {
	if (d->redraw) {
		d->mutex.lock();
		initializeTextures();
		d->redraw = false;
		d->mutex.unlock();
		setGeometryDirty();
	}
}

void MpOsdItem::initializeTextures() {
	if (!d->empty) {
		glBindTexture(GL_TEXTURE_2D, texture(0));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, d->stride, d->osdRect.height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, d->luma.data());
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, texture(1));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, d->stride, d->osdRect.height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, d->alpha.data());
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	setGeometryDirty();
}

const char *MpOsdItem::fragmentShader() const {
	const char *shader = (R"(
		uniform sampler2D tex_y, tex_a;
		varying highp vec2 qt_TexCoord;
		void main() {
			float luma = texture2D(tex_y, qt_TexCoord).x;
			float alpha = texture2D(tex_a, qt_TexCoord).x;
			gl_FragColor = vec4(luma, luma, luma, alpha);
		}
	)");
	return shader;
}
