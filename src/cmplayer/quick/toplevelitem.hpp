#ifndef TOPLEVELITEM_HPP
#define TOPLEVELITEM_HPP

#include "stdafx.hpp"
#include "simplevertexitem.hpp"

class TopLevelItem : public SimpleVertexItem {
    Q_OBJECT
public:
    TopLevelItem(QQuickItem *parent = nullptr);
    ~TopLevelItem();
    bool filteredMousePressEvent() const;
    void resetMousePressEventFilterState();
    Q_INVOKABLE void check();
    bool updateVertexOnGeometryChanged() const override { return true; }
    int vertexCount() const override { return 4; }
private:
    void updateVertex(Vertex *vertex) override;
    GLenum drawingMode() const override final { return GL_TRIANGLE_STRIP; }
    void itemChange(ItemChange change, const ItemChangeData &data);
    void mousePressEvent(QMouseEvent *event) override;
    struct Data;
    Data *d;
};

#endif // TOPLEVELITEM_HPP
