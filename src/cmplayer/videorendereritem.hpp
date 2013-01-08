#ifndef VIDEORENDERERITEM_HPP
#define VIDEORENDERERITEM_HPP

#include "stdafx.hpp"
#include "videoframe.hpp"
#include "colorproperty.hpp"
#include "mpmessage.hpp"
#include "texturerendereritem.hpp"


class LetterboxItem : public QQuickItem {
	Q_OBJECT
public:
	LetterboxItem(QQuickItem *parent = 0);
	QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data);
	void set(const QRectF &outer, const QRectF &inner);
private:
	QRectF m_outer, m_inner;
	bool m_rectChanged;
};

class VideoRendererItem;		class PlayEngine;

void plug(PlayEngine *engine, VideoRendererItem *video);
void unplug(PlayEngine *engine, VideoRendererItem *video);

class VideoRendererItem : public TextureRendererItem, public MpMessage {
	Q_OBJECT
public:
	enum Effect {
		NoEffect			= 0,
		FlipVertically		= 1 << 0,
		FlipHorizontally	= 1 << 1,
		Grayscale			= 1 << 2,
		InvertColor			= 1 << 3,
		Blur				= 1 << 4,
		Sharpen				= 1 << 5,
		RemapLuma			= 1 << 6,
		IgnoreEffect		= 1 << 8
	};
	Q_DECLARE_FLAGS(Effects, Effect)
	static const int FilterEffects = InvertColor | RemapLuma;
	static const int KernelEffects = Blur | Sharpen;
	VideoRendererItem(QQuickItem *parent = 0);
	~VideoRendererItem();
	double targetAspectRatio() const;
	double targetCropRatio(double fallback) const;
	double targetCropRatio() const {return targetCropRatio(targetAspectRatio());}
	double itemAspectRatio() const {return width()/height();}
	QPoint offset() const;
	quint64 drawnFrames() const;
	const VideoFormat &format() const;
	const ColorProperty &color() const;
	double aspectRatio() const;
	double cropRatio() const;
	int alignment() const;
	Effects effects() const;
	StreamList streams() const;
	int currentStreamId() const;
	int outputWidth() const;
	QSize sizeHint() const;
	QSizeF size() const {return QSizeF(width(), height());}
public slots:
	void setAlignment(int alignment);
	void setEffects(Effects effect);
	void setColor(const ColorProperty &prop);
	void setOffset(const QPoint &offset);
	void setCurrentStream(int id);
	void setCropRatio(double ratio);
	void setAspectRatio(double ratio);
signals:
	void effectsChanged(Effects effects);
	void offsetChanged(const QPoint &pos);
	void formatChanged(const VideoFormat &format);
private slots:
	void handleAboutToPlay() {}
	void handleAboutToOpen();
private:
	QByteArray shader() const;
	VideoFrame &getNextFrame() const;

	const char *fragmentShader() const;
	void link(QOpenGLShaderProgram *program);
	void bind(const RenderState &state, QOpenGLShaderProgram *program);
	void updateTexturedPoint2D(TexturedPoint2D *tp);
	bool prepare(const VideoFormat &format);
	bool beforeUpdate() override;
	void next();
	bool parse(const Id &id);
	bool parse(const QString &/*line*/) {return false;}
	void updateGeometry();
	static bool isSameRatio(double r1, double r2) {return (r1 < 0.0 && r2 < 0.0) || qFuzzyCompare(r1, r2);}
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	struct Data;
	Data *d;

	friend class VideoOutput;
	friend void plug(PlayEngine *engine, VideoRendererItem *video);
	friend void unplug(PlayEngine *engine, VideoRendererItem *video);
};

extern VideoRendererItem *item;

#endif // VIDEORENDERERITEM_HPP
