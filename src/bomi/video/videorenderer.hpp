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
    auto sizeHint() const -> QSize;
    auto overlay() const -> QQuickItem*;
    auto hasFrame() const -> bool;
    auto frameRect(const QRectF &area) const -> QRectF;
    auto overlayOnLetterbox() const -> bool;
    auto mapToVideo(const QPointF &pos) -> QPointF;
    auto isOpaque() const -> bool override { return true; }


    auto setFlipped(bool horizontal, bool vertical) -> void;
    auto setAspectRatio(double ratio) -> void;
    auto setOverlay(GeometryItem *overlay) -> void;
    auto setOverlayOnLetterbox(bool letterbox) -> void;
    auto setAlignment(Qt::Alignment alignment) -> void;
    auto setOffset(const QPoint &offset) -> void;
    auto setCropRatio(double ratio) -> void;

signals:
    void offsetChanged(const QPoint &pos);
    void screenRectChanged(const QRectF &rect);
    void overlayOnLetterboxChanged(bool on);
    void alignmentChanged(int alignment);
protected:
    auto updateVertexOnGeometryChanged() const -> bool override { return true; }
private:
    auto updateForNewFrame(const QSize &displaySize) -> void;
    auto setRenderFrameFunction(const RenderFrameFunc &func) -> void;

    auto initializeGL() -> void override;
    auto finalizeGL() -> void override;
    auto customEvent(QEvent *event) -> void override;
    auto updatePolish() -> void override;
    auto updateVertex(Vertex *vertex) -> void override;
    auto geometryChanged(const QRectF &new_, const QRectF&) -> void override;
    auto updateTexture(OpenGLTexture2D *texture) -> void override;
    struct Data;
    Data *d;
    friend class PlayEngine;
};

#endif // VIDEORENDERERITEM_HPP
