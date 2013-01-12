#include "mposditem.hpp"

struct OsdData {
	int x = 0, y = 0, w = 0, h = 0, stride = 0;
	uchar *src = nullptr, *srca = nullptr;
};

struct MpOsdItem::Data {
	int loc_tex_luma = 0;
	QByteArray luma;
	bool redraw = false, show = false;
	QRectF vertextRect = {.0, .0, .0, .0}, textureRect = {0.0, 0.0, 1.0, 1.0};
	QRect osdRect = {0, 0, 0, 0};
	OsdData data;
	QMutex mutex;
	QSize frameSize = {1, 1};
	bool frameChanged = false;
};

MpOsdItem::MpOsdItem(QQuickItem *parent)
: TextureRendererItem(1, parent), d(new Data) {
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
//	setVisible(d->mask);
}

void MpOsdItem::customEvent(QEvent *event) {
	TextureRendererItem::customEvent(event);
	if (event->type() == ShowEvent) {
		setVisible(d->show = true);
		update();
	} else if (event->type() == HideEvent) {
		setVisible(d->show = false);
//		update();
	}
}

void MpOsdItem::link(QOpenGLShaderProgram *program) {
	TextureRendererItem::link(program);
	d->loc_tex_luma = program->uniformLocation("tex_luma");
}

void MpOsdItem::bind(const RenderState &state, QOpenGLShaderProgram *program) {
	TextureRendererItem::bind(state, program);
	program->setUniformValue(d->loc_tex_luma, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture(0));
}

void MpOsdItem::updateTexturedPoint2D(TexturedPoint2D *tp) {
	if (d->show) {
		const auto exp_x = width()/d->frameSize.width();
		const auto exp_y = height()/d->frameSize.height();
		const QPointF pos(d->osdRect.x()*exp_x, d->osdRect.y()*exp_y);
		const QSizeF size(d->osdRect.width()*exp_x, d->osdRect.height()*exp_y);
		set(tp, QRectF(pos, size), d->textureRect);
	}
}

void MpOsdItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
	TextureRendererItem::geometryChanged(newGeometry, oldGeometry);
	setGeometryDirty();
	update();
}

void MpOsdItem::setFrameChanged() {
	d->frameChanged = true;
}

QMutex &MpOsdItem::mutex() const {
	return d->mutex;
}

void MpOsdItem::draw(int x, int y, int w, int h, uchar *src, uchar *srca, int stride) {
	const int len = h*stride;
	if (len > 0 && (d->data.x != x || d->data.y != y || d->data.w != w || d->data.h != h
			|| d->data.src != src || d->data.srca != srca || d->data.stride != stride)) {
		d->mutex.lock();
		d->data.x = x;	d->data.y = y;	d->data.w = w;	d->data.h = h;
		d->data.src = src;	d->data.srca = srca;	d->data.stride = stride;

		d->osdRect = QRect(x, y, w, h);
		d->luma.resize(len << 1);
		char *data = d->luma.data();
		for (int i=0; i<len; ++i) {
			*data++ = src[i];
			*data++ = -srca[i];
		}
		d->textureRect.setWidth((double)w/(double)stride);
		d->redraw = true;
		d->mutex.unlock();
	}
	qApp->postEvent(this, new QEvent((QEvent::Type)(len > 0 ? ShowEvent : HideEvent)));
}


void MpOsdItem::beforeUpdate() {
	if (d->redraw) {
		initializeTextures();
		d->redraw = false;
	}
}

void MpOsdItem::initializeTextures() {
	d->mutex.lock();
	glBindTexture(GL_TEXTURE_2D, texture(0));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, d->data.stride, d->osdRect.height(), 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, d->luma.data());
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	d->mutex.unlock();
	setGeometryDirty();
}

const char *MpOsdItem::fragmentShader() const {
	const char *shader = (R"(
		uniform sampler2D tex_luma;
		varying highp vec2 qt_TexCoord;
		void main() {
			gl_FragColor = texture2D(tex_luma, qt_TexCoord);
		}
	)");
	return shader;
}
