#ifndef BUSYICONITEM_HPP
#define BUSYICONITEM_HPP

#include "quick/simpletextureitem.hpp"

class BusyIconItem: public SimpleTextureItem {
    Q_OBJECT
    Q_PROPERTY(QColor lightColor READ lightColor WRITE setLightColor NOTIFY lightColorChanged)
    Q_PROPERTY(QColor darkColor READ darkColor WRITE setDarkColor NOTIFY darkColorChanged)
    Q_PROPERTY(qreal thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
public:
    BusyIconItem(QQuickItem *parent = 0);
    ~BusyIconItem();
    auto darkColor() const -> QColor;
    auto lightColor() const -> QColor;
    auto thickness() const -> qreal;
    auto setDarkColor(const QColor &color) -> void;
    auto setLightColor(const QColor &color) -> void;
    auto setThickness(qreal thickness) -> void;
    auto isRunning() const -> bool;
    auto setRunning(bool running) -> void;
signals:
    void thicknessChanged();
    void lightColorChanged();
    void darkColorChanged();
    void runningChanged();
private:
    auto initializeGL() -> void;
    auto finalizeGL() -> void;
    auto drawingMode() const -> GLenum { return GL_TRIANGLES; }
    auto updatePolish() -> void override;
    void updateTexture(OpenGLTexture2D *texture) final;
    auto geometryChanged(const QRectF &new_, const QRectF &o) -> void override;
    auto itemChange(ItemChange change, const ItemChangeData &) -> void override;
    struct Data;
    Data *d;
};

#endif // BUSYICONITEM_HPP
