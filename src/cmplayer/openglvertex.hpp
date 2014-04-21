#ifndef OPENGLVERTEX_HPP
#define OPENGLVERTEX_HPP

#include "stdafx.hpp"

namespace OGL {

using AttrData = QSGGeometry::Attribute;
using AttrInfo = QSGGeometry::AttributeSet;

struct ColorAttr {
    uchar r, g, b, a;
    ColorAttr &operator = (const QColor &rhs) {
        r = rhs.red();
        g = rhs.green();
        b = rhs.blue();
        a = rhs.alpha();
        return *this;
    }
    void set(uchar _r, uchar _g, uchar _b, uchar _a) {
        r = _r; g = _g; b = _b; a = _a;
    }
    static inline AttrData data(int index) {
        return AttrData::create(index, 4, GL_UNSIGNED_BYTE, false);
    }
};

template<class T> using VecIt = typename QVector<T>::iterator;

struct CoordAttr {
    float x, y;
    CoordAttr &operator = (const QPointF &rhs) {
        x = rhs.x(); y = rhs.y(); return *this;
    }
    static inline AttrData data(int index, bool isPos = false) {
        return AttrData::create(index, 2, GL_FLOAT, isPos);
    }
    void set(float x, float y) { this->x = x; this->y = y; }

    template<typename T>
    static inline VecIt<T> fillTriangleStrip(VecIt<T> it, CoordAttr T::*attr,
                                                const QPointF &tl,
                                                const QPointF &br) {
        (it->*attr).set(tl.x(), tl.y()); ++it;
        (it->*attr).set(tl.x(), br.y()); ++it;
        (it->*attr).set(br.x(), tl.y()); ++it;
        (it->*attr).set(br.x(), br.y()); ++it;
        return it;
    }

    template<typename T>
    static inline VecIt<T> fillTriangles(VecIt<T> it, CoordAttr T::*attr,
                                            const QPointF &tl,
                                            const QPointF &br) {
        (it->*attr).set(tl.x(), tl.y()); ++it;
        (it->*attr).set(br.x(), tl.y()); ++it;
        (it->*attr).set(tl.x(), br.y()); ++it;
        (it->*attr).set(tl.x(), br.y()); ++it;
        (it->*attr).set(br.x(), br.y()); ++it;
        (it->*attr).set(br.x(), tl.y()); ++it;
        return it;
    }
    template<typename T>
    static inline VecIt<T> fillTriangleStrip(VecIt<T> it, CoordAttr T::*attr1,
                                                const QPointF &tl1,
                                                const QPointF &br1,
                                                CoordAttr T::*attr2,
                                                const QPointF &tl2,
                                                const QPointF &br2) {
        (it->*attr1).set(tl1.x(), tl1.y());
        (it->*attr2).set(tl2.x(), tl2.y()); ++it;
        (it->*attr1).set(tl1.x(), br1.y());
        (it->*attr2).set(tl2.x(), br2.y()); ++it;
        (it->*attr1).set(br1.x(), tl1.y());
        (it->*attr2).set(br2.x(), tl2.y()); ++it;
        (it->*attr1).set(br1.x(), br1.y());
        (it->*attr2).set(br2.x(), br2.y()); ++it;
        return it;
    }
    template<typename T>
    static inline VecIt<T> fillTriangles(VecIt<T> it, CoordAttr T::*attr1,
                                            const QPointF &tl1,
                                            const QPointF &br1,
                                            CoordAttr T::*attr2,
                                            const QPointF &tl2,
                                            const QPointF &br2) {
        (it->*attr1).set(tl1.x(), tl1.y());
        (it->*attr2).set(tl2.x(), tl2.y()); ++it;
        (it->*attr1).set(br1.x(), tl1.y());
        (it->*attr2).set(br2.x(), tl2.y()); ++it;
        (it->*attr1).set(tl1.x(), br1.y());
        (it->*attr2).set(tl2.x(), br2.y()); ++it;
        (it->*attr1).set(tl1.x(), br1.y());
        (it->*attr2).set(tl2.x(), br2.y()); ++it;
        (it->*attr1).set(br1.x(), br1.y());
        (it->*attr2).set(br2.x(), br2.y()); ++it;
        (it->*attr1).set(br1.x(), tl1.y());
        (it->*attr2).set(br2.x(), tl2.y()); ++it;
        return it;
    }
};

struct PositionVertex {
    using Type = PositionVertex;
    CoordAttr position;
    static const AttrInfo &info() {
        static const auto data = CoordAttr::data(0, true);
        static const AttrInfo info = { 1, sizeof(PositionVertex), &data };
        return info;
    }
    static QList<QByteArray> names() {
        return QList<QByteArray>() << "aPosition";
    }
    static inline VecIt<Type> fillAsTriangleStrip(VecIt<Type> it,
                                                  const QPointF &tl,
                                                  const QPointF &br) {
        return CoordAttr::fillTriangleStrip(it, &Type::position, tl, br);
    }
    static inline VecIt<Type> fillAsTriangles(VecIt<Type> it,
                                              const QPointF &tl,
                                              const QPointF &br) {
        return CoordAttr::fillTriangles(it, &Type::position, tl, br);
    }
};

struct TextureVertex {
    using Type = TextureVertex;
    CoordAttr position, texCoord;
    static const AttrInfo &info() {
        static const AttrData data[] = {
            CoordAttr::data(0, true),
            CoordAttr::data(1, false)
        };
        static const AttrInfo info = { 2, sizeof(TextureVertex), data };
        return info;
    }
    static QList<QByteArray> names() {
        return QList<QByteArray>() << "aPosition" << "aTexCoord";
    }
    static inline VecIt<Type> fillAsTriangleStrip(VecIt<Type> it,
                                                  const QPointF &tl,
                                                  const QPointF &br,
                                                  const QPointF &ttl,
                                                  const QPointF &tbr) {
        return CoordAttr::fillTriangleStrip(it, &Type::position, tl, br,
                                            &Type::texCoord, ttl, tbr);
    }
    static inline VecIt<Type> fillAsTriangles(VecIt<Type> it,
                                              const QPointF &tl,
                                              const QPointF &br,
                                              const QPointF &ttl,
                                              const QPointF &tbr) {
        return CoordAttr::fillTriangles(it, &Type::position, tl, br,
                                        &Type::texCoord, ttl, tbr);
    }
};

struct ColorVertex {
    CoordAttr position;
    ColorAttr color;
    static const AttrInfo &info() {
        static const AttrData data[] = {
            CoordAttr::data(0, true),
            ColorAttr::data(1)
        };
        static const AttrInfo info = { 2, sizeof(ColorVertex), data };
        return info;
    }
    static QList<QByteArray> names() {
        return QList<QByteArray>() << "aPosition" << "aColor";
    }
};

struct TextureColorVertex {
    CoordAttr position, texCoord;
    ColorAttr color;
    static const AttrInfo &info() {
        static const AttrData data[] = {
            CoordAttr::data(0, true),
            CoordAttr::data(1, false),
            ColorAttr::data(2)
        };
        static const AttrInfo info = { 3, sizeof(TextureColorVertex), data };
        return info;
    }
    static QList<QByteArray> names() {
        return QList<QByteArray>() << "aPosition" << "aTexCoord" << "aColor";
    }
};

}


#endif // OPENGLVERTEX_HPP
