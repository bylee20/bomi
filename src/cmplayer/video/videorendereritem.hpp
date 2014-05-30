#ifndef VIDEORENDERERITEM_HPP
#define VIDEORENDERERITEM_HPP

#include "quick/highqualitytextureitem.hpp"
#include "enum/videoeffect.hpp"

class VideoColor;                       class Kernel3x3;
enum class DeintMethod;                 enum class ColorRange;

template<class T> class VideoImageCache;
class VideoFramebufferObject;           class MpOsdBitmap;

class VideoRendererItem : public HighQualityTextureItem {
    Q_OBJECT
    using Cache = VideoImageCache<VideoFramebufferObject>;
    using OsdCache = VideoImageCache<MpOsdBitmap>;
public:
    VideoRendererItem(QQuickItem *parent = 0);
    ~VideoRendererItem();
    auto screenRect() const -> QRectF;
    auto offset() const -> QPoint;
    auto aspectRatio() const -> double;
    auto cropRatio() const -> double;
    auto alignment() const -> int;
    auto effects() const -> VideoEffects;
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
    auto setEffects(VideoEffects effects) -> void;
    auto setOffset(const QPoint &offset) -> void;
    auto setCropRatio(double ratio) -> void;
    auto kernel() const -> const Kernel3x3&;
    auto osdSize() const -> QSize;
signals:
    void transferred();
    void frameImageObtained(const QImage &image) const;
    void effectsChanged(VideoEffects effects);
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
