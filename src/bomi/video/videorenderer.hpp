#ifndef VIDEORENDERERITEM_HPP
#define VIDEORENDERERITEM_HPP

#include "quick/opengldrawitem.hpp"
#include "opengl/openglvertex.hpp"
#include <functional>

class OpenGLFramebufferObject;
using RenderFrameFunc = std::function<void(OpenGLFramebufferObject*)>;

class VideoRenderer : public ShaderRenderItem<OGL::TextureVertex> {
    Q_OBJECT
public:
    VideoRenderer(QQuickItem *parent = 0);
    ~VideoRenderer();
    auto screenRect() const -> QRectF;
    auto offset() const -> QPoint;
    auto aspectRatio() const -> double;
    auto outputAspectRatio() const -> double;
    auto cropRatio() const -> double;
    auto alignment() const -> Qt::Alignment;
    auto sizeHint() const -> QSize;
    auto overlay() const -> QQuickItem*;
    auto hasFrame() const -> bool;
    auto frameRect(const QRectF &area) const -> QRectF;
    auto overlayOnLetterbox() const -> bool;
    auto mapToVideo(const QPointF &pos) -> QPointF;
    auto isOpaque() const -> bool final { return true; }
    auto setFlipped(bool horizontal, bool vertical) -> void;
    auto setAspectRatio(double ratio) -> void;
    auto setOverlay(GeometryItem *overlay) -> void;
    auto setOverlayOnLetterbox(bool letterbox) -> void;
    auto setAlignment(Qt::Alignment alignment) -> void;
    auto setOffset(const QPoint &offset) -> void;
    auto setCropRatio(double ratio) -> void;
    auto setRenderFrameFunction(const RenderFrameFunc &func) -> void;
    auto updateForNewFrame(const QSize &displaySize) -> void;
signals:
    void offsetChanged(const QPoint &pos);
    void screenRectChanged(const QRectF &rect);
    void overlayOnLetterboxChanged(bool on);
    void alignmentChanged(int alignment);
private:
    auto initializeGL() -> void final;
    auto finalizeGL() -> void final;
    auto initializeVertex(Vertex *vertex) const -> void final;
    auto createShader() const -> ShaderIface* final;
    auto createData() const -> ShaderData* final;
    auto updateData(ShaderData *data) -> void final;
    auto drawingMode() const -> GLenum final { return GL_TRIANGLE_STRIP; }
    auto type() const -> Type* final { static Type type; return &type; }
    auto vertexCount() const -> int final { return 4; }
    auto updateVertex(Vertex *vertex) -> void final;
    auto updateVertexOnGeometryChanged() const -> bool final { return true; }

    auto geometryChanged(const QRectF &new_, const QRectF&) -> void final;
    auto updatePolish() -> void final;
    auto customEvent(QEvent *event) -> void final;

    struct VideoShaderData;
    struct VideoShaderIface;

    struct Data;
    Data *d;
};

#endif // VIDEORENDERERITEM_HPP
