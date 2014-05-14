#ifndef VIDEORENDERERITEM_HPP
#define VIDEORENDERERITEM_HPP

#include "stdafx.hpp"
#include "skin.hpp"
#include "quick/highqualitytextureitem.hpp"

class VideoColor;                       class Kernel3x3;
enum class DeintMethod;                 enum class ColorRange;

template<class T> class VideoImageCache;
class VideoFramebufferObject;           class MpOsdBitmap;

class VideoRendererItem : public HighQualityTextureItem {
    Q_OBJECT
    using Cache = VideoImageCache<VideoFramebufferObject>;
    using OsdCache = VideoImageCache<MpOsdBitmap>;
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
    auto screenRect() const -> QRectF;
    auto offset() const -> QPoint;
    auto aspectRatio() const -> double;
    auto cropRatio() const -> double;
    auto alignment() const -> int;
    auto effects() const -> Effects;
    auto sizeHint() const -> QSize;
    auto setAspectRatio(double ratio) -> void;
    auto setOverlay(GeometryItem *overlay) -> void;
    auto overlay() const -> QQuickItem*;
    auto present(const Cache &cache) -> void;
    auto present(const Cache &cache, const OsdCache &osd) -> void;
    auto hasFrame() const -> bool;
    auto requestFrameImage() const -> void;
    auto frameRect(const QRectF &area) const -> QRectF;
    auto setKernel(const Kernel3x3 &blur, const Kernel3x3 &sharpen) -> void;
    auto setOverlayOnLetterbox(bool letterbox) -> void;
    auto overlayInLetterbox() const -> bool;
    auto mapToVideo(const QPointF &pos) -> QPointF;
    auto updateVertexOnGeometryChanged() const -> bool override { return true; }
    auto isOpaque() const -> bool override { return true; }
    auto setAlignment(int alignment) -> void;
    auto setEffects(Effects effect) -> void;
    auto setOffset(const QPoint &offset) -> void;
    auto setCropRatio(double ratio) -> void;
    auto kernel() const -> const Kernel3x3&;
    auto osdSize() const -> QSize;
signals:
    void transferred();
    void frameImageObtained(QImage image) const;
    void effectsChanged(Effects effects);
    void offsetChanged(const QPoint &pos);
    void screenRectChanged(const QRectF &rect);
    void frameRectChanged(const QRectF &rect);
    void kernelChanged(const Kernel3x3 &kernel);
    void osdSizeChanged(const QSize &size);
private:
    auto afterUpdate() -> void;
    auto initializeGL() -> void override;
    auto finalizeGL() -> void override;
    auto customEvent(QEvent *event) -> void override;
    auto updateVertex(Vertex *vertex) -> void override;
    auto updateTexture(OpenGLTexture2D *texture) -> void override;
    struct Data;
    Data *d;
    QPoint m_mouse;
    friend class VideoOutput;
};

#endif // VIDEORENDERERITEM_HPP
