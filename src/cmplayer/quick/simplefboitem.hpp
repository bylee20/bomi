#ifndef SIMPLEFBOITEM_HPP
#define SIMPLEFBOITEM_HPP

#include "simpletextureitem.hpp"

class SimpleFboItem : public SimpleTextureItem {
    Q_OBJECT
public:
    SimpleFboItem(QQuickItem *parent = nullptr);
    auto targetSize() const -> QSize { return m_targetSize; }
    virtual auto imageSize() const -> QSize { return targetSize(); }
    auto forceUpdateTargetSize() -> void { m_forced = true; }
    auto updateVertexOnGeometryChanged() const -> bool override { return true; }
signals:
    void targetSizeChanged(const QSize &size);
protected:
    auto forceRepaint() -> void { reserve(UpdateMaterial); }
    auto finalizeGL() -> void { SimpleTextureItem::finalizeGL(); _Delete(m_fbo); }
    auto geometryChanged(const QRectF &new_, const QRectF &old) -> void override;
    virtual void paint(OpenGLFramebufferObject *fbo) = 0;
private:
    void updateVertex(Vertex *vertex) final;
    void updateTexture(OpenGLTexture2D *texture) final;
    QTimer m_sizeChecker;
    QSize m_targetSize{0, 0}, m_prevSize{0, 0};
    OpenGLFramebufferObject *m_fbo = nullptr;
    bool m_forced = true;
};

#endif // SIMPLEFBOITEM_HPP
