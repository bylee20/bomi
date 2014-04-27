#ifndef VIDEORENDERERITEM_HPP
#define VIDEORENDERERITEM_HPP

#include "stdafx.hpp"
#include "skin.hpp"
#include "quick/highqualitytextureitem.hpp"

class DeintInfo;                class OpenGLFramebufferObject;
class VideoRendererItem;        class VideoColor;
class VideoFrame;                class VideoFormat;
class MpOsdItem;                class OpenGLTexture;
enum class DeintMethod;
enum class ColorRange;

class VideoRendererItem : public HighQualityTextureItem {
    Q_OBJECT
    static bool isSameRatio(double r1, double r2) {return (r1 < 0.0 && r2 < 0.0) || qFuzzyCompare(r1, r2);}
public:
    enum Effect {
        NoEffect         = 0,
        FlipVertically   = 1 << 0,
        FlipHorizontally = 1 << 1,
        Grayscale        = 1 << 2,
        InvertColor      = 1 << 3,
        Blur             = 1 << 4,
        Sharpen          = 1 << 5,
        Disable          = 1 << 8
    };
    Q_DECLARE_FLAGS(Effects, Effect)
    static const int KernelEffects = Blur | Sharpen;
    static const int ColorEffects = Grayscale | InvertColor;
    static const int ShaderEffects = KernelEffects | ColorEffects;
    VideoRendererItem(QQuickItem *parent = 0);
    ~VideoRendererItem();
    double targetAspectRatio() const;
    double targetCropRatio(double fallback) const;
    double targetCropRatio() const {return targetCropRatio(targetAspectRatio());}
    double itemAspectRatio() const {return width()/height();}
    QRectF screenRect() const;
    QPoint offset() const;
    quint64 drawnFrames() const;
    const VideoColor &color() const;
    double aspectRatio() const;
    double cropRatio() const;
    int alignment() const;
    double avgfps() const;
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
    void setKernel(int blur_c, int blur_n, int blur_d,
                   int sharpen_c, int sharpen_n, int sharpen_d);
    double delay() const;
    void setDeintMethod(DeintMethod method);
    void setOverlayOnLetterbox(bool letterbox);
    bool overlayInLetterbox() const;
    void setChromaUpscaler(InterpolatorType tpe);
    InterpolatorType chromaUpscaler() const;
    void setRange(ColorRange range);
    ColorRange range() const;
    int droppedFrames() const;
    void reset();
    QPointF mapToVideo(const QPointF &pos);
    void setMousePosition(const QPoint &pos) { m_mouse = pos; }
    const QPoint &mousePosition() const { return m_mouse; }
    bool updateVertexOnGeometryChanged() const override { return true; }
    bool isOpaque() const override { return true; }
public slots:
    void setAlignment(int alignment);
    void setEffects(Effects effect);
    void setColor(const VideoColor &prop);
    void setOffset(const QPoint &offset);
    void setCropRatio(double ratio);
signals:
    void droppedFramesChanged(int dropped);
    void frameImageObtained(QImage image) const;
    void effectsChanged(Effects effects);
    void offsetChanged(const QPoint &pos);
    void screenRectChanged(const QRectF &rect);
    void frameRectChanged(const QRectF &rect);
private:
    void afterUpdate();
    void initializeGL() override;
    void finalizeGL() override;
    void customEvent(QEvent *event) override;
    void updateVertex(Vertex *vertex) override;
    void updateTexture(OpenGLTexture2D *texture) override;
    MpOsdItem *mpOsd() const;
    struct Data;
    Data *d;
    QPoint m_mouse;
    friend class VideoOutput;
};

#endif // VIDEORENDERERITEM_HPP
