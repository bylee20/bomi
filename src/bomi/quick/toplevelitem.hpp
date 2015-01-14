#ifndef TOPLEVELITEM_HPP
#define TOPLEVELITEM_HPP

#include "simplevertexitem.hpp"

class TopLevelItem : public SimpleVertexItem {
    Q_OBJECT
public:
    TopLevelItem(QQuickItem *parent = nullptr);
    ~TopLevelItem();
    auto filteredMousePressEvent() const -> bool;
    auto resetMousePressEventFilterState() -> void;
    auto updateVertexOnGeometryChanged() const -> bool override { return true; }
    auto vertexCount() const -> int override { return 4; }
    Q_INVOKABLE void check();
private:
    auto updateVertex(Vertex *vertex) -> void override;
    auto drawingMode() const -> GLenum final { return GL_TRIANGLE_STRIP; }
    auto itemChange(ItemChange change, const ItemChangeData &data) -> void;
    auto mousePressEvent(QMouseEvent *event) -> void override;
    struct Data;
    Data *d;
};

#endif // TOPLEVELITEM_HPP
