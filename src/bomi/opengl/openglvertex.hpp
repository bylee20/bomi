#ifndef OPENGLVERTEX_HPP
#define OPENGLVERTEX_HPP

#include <QSGGeometry>

namespace OGL {

using AttrData = QSGGeometry::Attribute;
using AttrInfo = QSGGeometry::AttributeSet;

struct ColorAttr {
    uchar r, g, b, a;
    auto set(const QColor &color) -> void {
        r = color.red();
        g = color.green();
        b = color.blue();
        a = color.alpha();
    }
    auto set(quint32 color) -> void {
        *reinterpret_cast<quint32*>(this) = color;
    }
    auto set(uchar _r, uchar _g, uchar _b, uchar _a) -> void {
        r = _r; g = _g; b = _b; a = _a;
    }
    auto toColor() const -> QColor { return QColor(r, g, b, a); }
    SIA data(int index) -> AttrData {
        return AttrData::create(index, 4, GL_UNSIGNED_BYTE, false);
    }
};

#define SET_ATTR_COLOR(prog, loc, type, mem) { prog->setAttributeBuffer(loc, \
    GL_UNSIGNED_BYTE, offsetof(type, mem), 4, sizeof(type)); }

#define SET_ATTR_COORD(prog, loc, type, mem) { prog->setAttributeBuffer(loc, \
    GL_FLOAT, offsetof(type, mem), 2, sizeof(type)); }

template<class T> using VecIt = typename QVector<T>::iterator;

struct CoordAttr {
    float x, y;
    SIA data(int index, bool isPos = false) -> AttrData {
        return AttrData::create(index, 2, GL_FLOAT, isPos);
    }
    auto toPoint() const -> QPointF { return { x, y }; }
    auto set(const QPointF &p) -> void { x = p.x(); y = p.y(); }
    auto set(float x, float y) -> void { this->x = x; this->y = y; }

    template<class T>
    static inline VecIt<T> fillTriangleStrip(VecIt<T> it, CoordAttr T::*attr,
                                                const QPointF &tl,
                                                const QPointF &br) {
        (it->*attr).set(tl.x(), tl.y()); ++it;
        (it->*attr).set(tl.x(), br.y()); ++it;
        (it->*attr).set(br.x(), tl.y()); ++it;
        (it->*attr).set(br.x(), br.y()); ++it;
        return it;
    }

    template<class T>
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
    template<class T>
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
    template<class T, class F>
    static inline VecIt<T> fillTriangles(VecIt<T> it, CoordAttr T::*attr1,
                                         const QPointF &tl1,
                                         const QPointF &br1,
                                         CoordAttr T::*attr2,
                                         const QPointF &tl2,
                                         const QPointF &br2, F f) {
        (it->*attr1).set(tl1.x(), tl1.y());
        (it->*attr2).set(tl2.x(), tl2.y()); f(it); ++it;
        (it->*attr1).set(br1.x(), tl1.y());
        (it->*attr2).set(br2.x(), tl2.y()); f(it); ++it;
        (it->*attr1).set(tl1.x(), br1.y());
        (it->*attr2).set(tl2.x(), br2.y()); f(it); ++it;
        (it->*attr1).set(tl1.x(), br1.y());
        (it->*attr2).set(tl2.x(), br2.y()); f(it); ++it;
        (it->*attr1).set(br1.x(), br1.y());
        (it->*attr2).set(br2.x(), br2.y()); f(it); ++it;
        (it->*attr1).set(br1.x(), tl1.y());
        (it->*attr2).set(br2.x(), tl2.y()); f(it); ++it;
        return it;
    }
    template<class T>
    static inline VecIt<T> fillTriangles(VecIt<T> it, CoordAttr T::*attr1,
                                         const QPointF &tl1,
                                         const QPointF &br1,
                                         CoordAttr T::*attr2,
                                         const QPointF &tl2,
                                         const QPointF &br2) {
        return fillTriangles(it, attr1, tl1, br1, attr2, tl2, br2,
                             [] (const VecIt<T>&) {});
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
    static QVector<QByteArray> names() {
        return QVector<QByteArray>() << "aPosition";
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
    static QVector<QByteArray> names() {
        return QVector<QByteArray>() << "aPosition" << "aTexCoord";
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
    static QVector<QByteArray> names() {
        return QVector<QByteArray>() << "aPosition" << "aColor";
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
    static QVector<QByteArray> names() {
        return QVector<QByteArray>() << "aPosition" << "aTexCoord" << "aColor";
    }
};

}


#endif // OPENGLVERTEX_HPP
