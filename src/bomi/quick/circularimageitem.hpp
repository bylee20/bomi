#ifndef CIRCULARIMAGE_HPP
#define CIRCULARIMAGE_HPP

#include "simpletextureitem.hpp"

class CircularImageItem : public SimpleTextureItem {
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal radian READ angle WRITE setAngle NOTIFY angleChanged)
    Q_PROPERTY(qreal degree READ degree WRITE setDegree NOTIFY angleChanged)
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged)
public:
    CircularImageItem(QQuickItem *parent = nullptr);
    ~CircularImageItem();
    auto source() const -> QUrl;
    auto setSource(const QUrl &url) -> void;
    auto vertexCount() const -> int override;
    auto updateVertexOnGeometryChanged() const -> bool override;
    auto drawingMode() const -> GLenum override;
    auto angle() const -> qreal;
    auto degree() const -> qreal { return qRadiansToDegrees(angle()); }
    auto setAngle(qreal rad) -> void;
    auto setDegree(qreal deg) -> void { setAngle(qDegreesToRadians(deg)); }
signals:
    void sourceChanged(const QUrl &url);
    void angleChanged();
private:
    auto type() const -> Type* override { static Type type; return &type; }
    auto geometryChanged(const QRectF &n, const QRectF &o) -> void override;
    auto initializeGL() -> void override;
    auto finalizeGL() -> void override;
    auto updateTexture(OpenGLTexture2D *texture) -> void override;
    auto updateVertex(Vertex *vertex) -> void override;
    struct Data;
    Data *d;
};

#endif // CIRCULARIMAGE_HPP
