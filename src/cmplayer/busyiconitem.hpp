#ifndef BUSYICONITEM_HPP
#define BUSYICONITEM_HPP

#include "stdafx.hpp"
#include "geometryitem.hpp"

class BusyIconItem: public SimpleTextureItem {
    Q_OBJECT
    Q_PROPERTY(QColor lightColor READ lightColor WRITE setLightColor NOTIFY lightColorChanged)
    Q_PROPERTY(QColor darkColor READ darkColor WRITE setDarkColor NOTIFY darkColorChanged)
    Q_PROPERTY(qreal thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
public:
    BusyIconItem(QQuickItem *parent = 0);
    ~BusyIconItem();
    QColor darkColor() const;
    QColor lightColor() const;
    qreal thickness() const;
    void setDarkColor(const QColor &color);
    void setLightColor(const QColor &color);
    void setThickness(qreal thickness);
    bool isRunning() const;
    void setRunning(bool running);
signals:
    void thicknessChanged();
    void lightColorChanged();
    void darkColorChanged();
    void runningChanged();
private:
    void initializeGL();
    void finalizeGL();
    GLenum drawingMode() const { return GL_TRIANGLES; }
    void updatePolish() override;
    void updateTexture(OpenGLTexture2D &texture) override final;
    void geometryChanged(const QRectF &new_, const QRectF &old) override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    struct Data;
    Data *d;
};

#endif // BUSYICONITEM_HPP
