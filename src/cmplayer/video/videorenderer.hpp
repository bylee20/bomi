#ifndef VIDEORENDERERITEM_HPP
#define VIDEORENDERERITEM_HPP

#include "quick/simpletextureitem.hpp"
#include <functional>

class OpenGLFramebufferObject;
using RenderFrameFunc = std::function<void(OpenGLFramebufferObject*)>;

class VideoRenderer : public SimpleTextureItem {
    Q_OBJECT
public:
    VideoRenderer(QQuickItem *parent = 0);
    ~VideoRenderer();
    auto screenRect() const -> QRectF;
    auto offset() const -> QPoint;
    auto aspectRatio() const -> double;
    auto cropRatio() const -> double;
    auto alignment() const -> Qt::Alignment;
    auto setFlipped(bool horizontal, bool vertical) -> void;
    auto sizeHint() const -> QSize;
    auto setAspectRatio(double ratio) -> void;
    auto setOverlay(GeometryItem *overlay) -> void;
    auto overlay() const -> QQuickItem*;
    auto hasFrame() const -> bool;
    auto requestFrameImage() const -> void;
    auto frameRect(const QRectF &area) const -> QRectF;
    auto setOverlayOnLetterbox(bool letterbox) -> void;
    auto overlayOnLetterbox() const -> bool;
    auto mapToVideo(const QPointF &pos) -> QPointF;
    auto updateVertexOnGeometryChanged() const -> bool override { return true; }
    auto isOpaque() const -> bool override { return true; }
    auto setAlignment(Qt::Alignment alignment) -> void;
    auto setOffset(const QPoint &offset) -> void;
    auto setCropRatio(double ratio) -> void;
    auto updateForNewFrame(const QSize &displaySize) -> void;
    auto setRenderFrameFunction(const RenderFrameFunc &func) -> void;
signals:
    void frameImageObtained(const QImage &video, const QImage &osd) const;
    void offsetChanged(const QPoint &pos);
    void screenRectChanged(const QRectF &rect);
    void overlayOnLetterboxChanged(bool on);
    void alignmentChanged(int alignment);
private:
    auto initializeGL() -> void override;
    auto finalizeGL() -> void override;
    auto customEvent(QEvent *event) -> void override;
    auto updatePolish() -> void override;
    auto updateVertex(Vertex *vertex) -> void override;
    auto geometryChanged(const QRectF &new_, const QRectF&) -> void override;
    auto updateTexture(OpenGLTexture2D *texture) -> void override;
    struct Data;
    Data *d;
};

#endif // VIDEORENDERERITEM_HPP
