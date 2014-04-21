#ifndef LETTERBOXITEM_HPP
#define LETTERBOXITEM_HPP

#include "stdafx.hpp"
#include "simplevertexitem.hpp"

class LetterboxItem : public SimpleVertexItem {
public:
    LetterboxItem(QQuickItem *parent = 0);
    bool set(const QRectF &outer, const QRectF &inner);
    const QRectF &screen() {return m_screen;}
private:
    void updatePolish();
    GLenum drawingMode() const override final { return GL_TRIANGLES; }
    QRectF m_outer, m_inner, m_screen;
};

#endif // LETTERBOXITEM_HPP
