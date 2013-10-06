#ifndef VIDEORENDERERITEM_HPP
#define VIDEORENDERERITEM_HPP

#include "stdafx.hpp"
#include "skin.hpp"
#include "texturerendereritem.hpp"

class DeintInfo;				class OpenGLFramebufferObject;
class VideoRendererItem;		class ColorProperty;
class VideoFrame;				class VideoFormat;
class MpOsdItem;				class OpenGLTexture;
enum class InterpolatorType;	enum class DeintMethod;

class VideoRendererItem : public TextureRendererItem {
	Q_OBJECT
	static bool isSameRatio(double r1, double r2) {return (r1 < 0.0 && r2 < 0.0) || qFuzzyCompare(r1, r2);}
public:
	enum Effect {
		NoEffect			= 0,
		FlipVertically		= 1 << 0,
		FlipHorizontally	= 1 << 1,
		Grayscale			= 1 << 2,
		InvertColor			= 1 << 3,
		Blur				= 1 << 4,
		Sharpen				= 1 << 5,
		Disable				= 1 << 8
	};
	Q_DECLARE_FLAGS(Effects, Effect)
	static const int KernelEffects = Blur | Sharpen;
	static const int ColorEffects = Grayscale | InvertColor;
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
	QSize sizeHint() const;
	QSizeF size() const {return QSizeF(width(), height());}
	QQuickItem *osd() const;
	void setAspectRatio(double ratio);
	void setOverlay(GeometryItem *overlay);
	QQuickItem *overlay() const;
	void present(const VideoFrame &frame);
	void present(const QImage &image);
	const VideoFrame &frame() const;
	bool isFramePended() const;
	bool hasFrame() const;
	void requestFrameImage() const;
	QRectF frameRect(const QRectF &area) const;
	void setKernel(int blur_c, int blur_n, int blur_d, int sharpen_c, int sharpen_n, int sharpen_d);
	int delay() const;
	void setDeintMethod(DeintMethod method);
	void swap(OpenGLTexture &texture, const VideoFormat &format);
	void rerender() override;
	GLuint texture() const;
	void setOverlayInLetterbox(bool letterbox);
	bool overlayInLetterbox() const;
public slots:
	void setAlignment(int alignment);
	void setEffects(Effects effect);
	void setColor(const ColorProperty &prop);
	void setOffset(const QPoint &offset);
	void setCropRatio(double ratio);
signals:
	void frameImageObtained(const QImage &image) const;
	void effectsChanged(Effects effects);
	void offsetChanged(const QPoint &pos);
	void screenRectChanged(const QRectF rect);
private:
	void initializeGL() override;
	void finalizeGL() override;
	void getCoords(QRectF &vertices, QRectF &texCoords) override;
	void prepare(QSGGeometryNode *node) override;
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
	void customEvent(QEvent *event) override;
	MpOsdItem *mpOsd() const;
	struct Data;
	Data *d;
	friend class VideoOutput;
};

extern VideoRendererItem *item;

#endif // VIDEORENDERERITEM_HPP
