#ifndef SIMPLEFBOITEM_HPP
#define SIMPLEFBOITEM_HPP

#include "simpletextureitem.hpp"

class SimpleFboItem : public SimpleTextureItem {
    Q_OBJECT
public:
    SimpleFboItem(QQuickItem *parent = nullptr);
    QSize targetSize() const { return m_targetSize; }
    virtual QSize imageSize() const { return targetSize(); }
    void forceUpdateTargetSize() { m_forced = true; }
    bool updateVertexOnGeometryChanged() const override { return true; }
signals:
    void targetSizeChanged(const QSize &size);
protected:
    void forceRepaint() { reserve(UpdateMaterial); }
    void finalizeGL() { SimpleTextureItem::finalizeGL(); _Delete(m_fbo); }
    void geometryChanged(const QRectF &new_, const QRectF &old) override;
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
