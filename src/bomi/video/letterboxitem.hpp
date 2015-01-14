#ifndef LETTERBOXITEM_HPP
#define LETTERBOXITEM_HPP

#include "quick/simplevertexitem.hpp"

class LetterboxItem : public SimpleVertexItem {
public:
    LetterboxItem(QQuickItem *parent = 0);
    auto set(const QRectF &outer, const QRectF &inner) -> bool;
    const QRectF &screen() {return m_screen;}
private:
    auto updatePolish() -> void;
    GLenum drawingMode() const override final { return GL_TRIANGLES; }
    QRectF m_outer, m_inner, m_screen;
};

#endif // LETTERBOXITEM_HPP
