#ifndef VIDEORENDERERITEM_HPP
#define VIDEORENDERERITEM_HPP

#include "quick/opengldrawitem.hpp"
#include "opengl/openglvertex.hpp"
#include "opengl/openglmisc.hpp"
#include <functional>

class OpenGLFramebufferObject;          enum class Rotation;
using Fbo = OpenGLFramebufferObject;
using RenderFrameFunc = std::function<void(Fbo*,Fbo*,const QMargins&)>;

struct VideoFrameOsdVertex {
    OGL::CoordAttr position, frameTexCoord, osdTexCoord;
    static const OGL::AttrInfo &info() {
        static const OGL::AttrData data[] = {
            OGL::CoordAttr::data(0, true),
            OGL::CoordAttr::data(1, false),
            OGL::CoordAttr::data(2, false)
        };
        static const OGL::AttrInfo info = { 3, sizeof(VideoFrameOsdVertex), data };
        return info;
    }
};

class VideoRenderer : public ShaderRenderItem<VideoFrameOsdVertex> {
    Q_OBJECT
    using Super = ShaderRenderItem<VideoFrameOsdVertex>;
    Q_PROPERTY(QRectF frameRect READ screenRect NOTIFY screenRectChanged)
public:
    VideoRenderer(QQuickItem *parent = 0);
    ~VideoRenderer();
    auto screenRect() const -> QRectF;
    auto offset() const -> QPointF;
    auto aspectRatio() const -> double;
    auto outputAspectRatio() const -> double;
    auto cropRatio() const -> double;
    auto alignment() const -> Qt::Alignment;
    auto sizeHint() const -> QSize;
    auto overlay() const -> QQuickItem*;
    auto rotation() const -> Rotation;
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
    auto setOffset(const QPointF &offset) -> void;
    auto setCropRatio(double ratio) -> void;
    auto setRotation(Rotation r) -> void;
    auto setRenderFrameFunction(const RenderFrameFunc &func) -> void;
    auto updateForNewFrame(const QSize &displaySize) -> void;
    auto setFramebufferObjectFormat(OGL::TextureFormat format) -> void;
    auto framebufferObjectFormat() const -> OGL::TextureFormat;
    auto isPortrait() const -> bool;
    auto setScalerEnabled(bool on) -> void;
    auto setOsdVisible(bool visible) -> void;
    auto updateAll() -> void;
    Q_INVOKABLE QRectF mapFromVideo(const QRect &rect);
signals:
    void offsetChanged(const QPointF &pos);
    void screenRectChanged(const QRectF &rect);
    void overlayOnLetterboxChanged(bool on);
    void alignmentChanged(int alignment);
private:
    auto initializeGL() -> void final;
    auto finalizeGL() -> void final;
    auto initializeVertex(Vertex *vertex) const -> void final;
    auto createNode() const -> QSGGeometryNode* final;
    auto createShader() const -> ShaderIface* final;
    auto createData() const -> ShaderData* final;
    auto updateData(ShaderData *data) -> void final;
    auto drawingMode() const -> GLenum final { return GL_TRIANGLE_STRIP; }
    auto type() const -> Type* final;
    auto vertexCount() const -> int final { return 4; }
    auto updateVertex(Vertex *vertex) -> void final;
    auto updateVertexOnGeometryChanged() const -> bool final { return true; }

    auto geometryChanged(const QRectF &new_, const QRectF&) -> void final;
    auto updatePolish() -> void final;
    auto customEvent(QEvent *event) -> void final;
    struct VideoShaderData;
    struct VideoShaderIface;
    struct Node;

    auto render(VideoShaderData *data) -> void;

    struct Data;
    Data *d;
};

#endif // VIDEORENDERERITEM_HPP
