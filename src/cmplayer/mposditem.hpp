#ifndef MPOSDITEM_HPP
#define MPOSDITEM_HPP

#include "stdafx.hpp"
#include "texturerendereritem.hpp"
#include "openglmisc.hpp"

enum EventType {EnqueueFrame = QEvent::User + 1, NextFrame, EmptyQueue, Rerender, UpdateDeint, Show, Hide, NewFrame };

struct sub_bitmaps;

class FramebufferObjectRendererItem : public HQTextureRendererItem {
	Q_OBJECT
public:
	FramebufferObjectRendererItem(QQuickItem *parent = nullptr)
	: HQTextureRendererItem(parent) {
		connect(&m_sizeChecker, &QTimer::timeout, [this] () {
			if (!_Change(m_prevSize, QSizeF(width(), height()).toSize())) {
				m_sizeChecker.stop();
				emit targetSizeChanged(m_targetSize = m_prevSize);
			}
		});
		m_sizeChecker.setInterval(300);
	}
	QSize targetSize() const { return m_targetSize; }
	virtual QSize imageSize() const { return targetSize(); }
	void forceUpdateTargetSize() { m_forced = true; }
	bool isOpaque() const override { return false; }
signals:
	void targetSizeChanged(const QSize &size);
protected:
	void forceRepaint() { m_repaint = true; update(); }
	void finalizeGL() { HQTextureRendererItem::finalizeGL(); _Delete(m_fbo); }
	virtual void paint(OpenGLFramebufferObject *fbo) = 0;
	void geometryChanged(const QRectF &newOne, const QRectF &old) {
		if (m_forced) {
			if (_Change(m_targetSize, newOne.size().toSize()))
				emit targetSizeChanged(m_targetSize);
			m_forced = false;
		}else
			m_sizeChecker.start();
		m_repaint = true;
		HQTextureRendererItem::geometryChanged(newOne, old);
	}
private:
	void prepare(QSGGeometryNode *node) final override {
		if (m_repaint) {
			const auto size = imageSize();
			if (!m_fbo || m_fbo->size() != size) {
				_Renew(m_fbo, size);
				setRenderTarget(m_fbo->texture());
				LOG_GL_ERROR_Q
			}
			paint(m_fbo);
			setGeometryDirty();
			node->markDirty(QSGNode::DirtyMaterial);
			m_repaint = false;
			LOG_GL_ERROR_Q
		}
	}
	QTimer m_sizeChecker;
	QSize m_targetSize{0, 0}, m_prevSize{0, 0};
	OpenGLFramebufferObject *m_fbo = nullptr;
	bool m_forced = true, m_repaint = true;
};

class MpOsdItem : public FramebufferObjectRendererItem {
	Q_OBJECT
public:
	MpOsdItem(QQuickItem *parent = nullptr);
	~MpOsdItem();
	void drawOn(sub_bitmaps *imgs);
	void present(bool redraw);
	void drawOn(QImage &frame);
//	QSize targetSize() const;
	void setImageSize(const QSize &size);
//	void forceUpdateTargetSize();
	QSize imageSize() const override;
private:
	void paint(OpenGLFramebufferObject *fbo) override;
	void initializeGL() override;
	void finalizeGL() override;
	void customEvent(QEvent *event) override;
//	void geometryChanged(const QRectF &newOne, const QRectF &old);
	struct Data;
	Data *d;
};

#endif // MPOSDITEM_HPP
