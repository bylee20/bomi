#ifndef VIDEORENDERERITEM_HPP
#define VIDEORENDERERITEM_HPP

#include "stdafx.hpp"
#include "skin.hpp"
#include "texturerendereritem.hpp"

class VideoRendererItem;		class ColorProperty;
class VideoFrame;				class VideoFormat;

class VideoRendererItem : public TextureRendererItem, public Skin {
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
	QRectF screenRect() const;
	QPoint offset() const;
	quint64 drawnFrames() const;
	const ColorProperty &color() const;
	double aspectRatio() const;
	double cropRatio() const;
	int alignment() const;
	double fps() const;
	Effects effects() const;
	int outputWidth() const;
	QSize sizeHint() const;
	QSizeF size() const {return QSizeF(width(), height());}
	void setVideoAspectRaito(double ratio);
	QQuickItem *osd() const;
	void setAspectRatio(double ratio);
	void quit();
	void setOverlay(QQuickItem *overlay);
	QQuickItem *overlay() const;
public slots:
	void setAlignment(int alignment);
	void setEffects(Effects effect);
	void setColor(const ColorProperty &prop);
	void setOffset(const QPoint &offset);
	void setCropRatio(double ratio);
signals:
	void effectsChanged(Effects effects);
	void offsetChanged(const QPoint &pos);
	void formatChanged(const VideoFormat &format);
	void screenRectChanged(const QRectF rect);
private: // for VideoOutput
	VideoFrame &getNextFrame() const;
	void next();
private:
	void initializeTextures();
	static QByteArray shader(int type);
    static void drawMpOsd(void *pctx, struct sub_bitmaps *imgs);
	const char *fragmentShader() const;
	void link(QOpenGLShaderProgram *program);
	void bind(const RenderState &state, QOpenGLShaderProgram *program);
	void updateTexturedPoint2D(TexturedPoint2D *tp);
	void beforeUpdate() override;
	void updateGeometry();
	static bool isSameRatio(double r1, double r2) {return (r1 < 0.0 && r2 < 0.0) || qFuzzyCompare(r1, r2);}
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	struct Data;
	Data *d;

	friend class VideoOutput;
};

extern VideoRendererItem *item;

#endif // VIDEORENDERERITEM_HPP
