#ifndef OPENGLDRAWITEM_HPP
#define OPENGLDRAWITEM_HPP

#include "geometryitem.hpp"

class OpenGLDrawItem : public GeometryItem {
    Q_OBJECT
public:
    using AttrSet = QSGGeometry::AttributeSet;
    struct ColorVertex { QPointF vertex; QColor color; };
    enum UpdateHint {
        UpdateGeometry = 1,
        UpdateMaterial = 2,
        UpdateAll = UpdateGeometry | UpdateMaterial
    };
    OpenGLDrawItem(QQuickItem *parent = nullptr);
    ~OpenGLDrawItem();
    auto devicePixelRatio() const -> double;
    auto logAt(const char *func) const -> QByteArray;
    auto polishAndUpdate() -> void { polish(); update(); }
    virtual auto rerender() -> void { reserve(UpdateAll); }
    static auto func() -> QOpenGLFunctions* { return context()->functions(); }
    static auto context() -> QOpenGLContext*;
protected:
    auto isInitialized() const -> bool { return m_init; }
    auto reserve(UpdateHint hint, bool update = true) -> void;
    auto isReserved(UpdateHint update) const -> bool;
    virtual auto initializeGL() -> void { }
    virtual auto finalizeGL() -> void { }
    virtual auto drawingMode() const -> GLenum { return GL_TRIANGLE_STRIP; }
    virtual auto attributeSet() const -> const AttrSet&;
    virtual auto vertexCount() const -> int { return 0; }
    virtual auto createGeometry() const -> QSGGeometry* = 0;
    virtual auto createMaterial() const -> QSGMaterial* = 0;
    virtual auto updateGeometry(QSGGeometry *geometry) -> QSGGeometry* = 0;
    virtual auto updateMaterial(QSGMaterial *material) -> QSGMaterial* = 0;
    virtual auto afterUpdate() -> void { }
private:
    auto tryInitGL() -> void;
    auto updatePaintNode(QSGNode *old, UpdatePaintNodeData *) -> QSGNode* final;
    QQuickWindow *m_win = nullptr;
    bool m_init = false;
    QSGGeometryNode *m_node = nullptr;
    int m_updates = 0;
};

inline auto OpenGLDrawItem::devicePixelRatio() const -> double
{ return m_win ? m_win->devicePixelRatio() : 1.0; }

inline auto OpenGLDrawItem::logAt(const char *func) const -> QByteArray
{ return QByteArray(metaObject()->className()) + "::" + func; }

inline auto OpenGLDrawItem::attributeSet() const -> const AttrSet&
{ return QSGGeometry::defaultAttributes_TexturedPoint2D(); }

inline auto OpenGLDrawItem::context() -> QOpenGLContext*
{ return QOpenGLContext::currentContext(); }

inline auto OpenGLDrawItem::reserve(UpdateHint hint, bool update) -> void
{ m_updates |= hint; if (update) this->update(); }

inline auto OpenGLDrawItem::isReserved(UpdateHint update) const -> bool
{ return m_updates & update; }

inline auto OpenGLDrawItem::tryInitGL() -> void
{ if (!m_init && context()) { initializeGL(); m_init = true; } }

/******************************************************************************/

template <class T>
class VertexDrawItem : public OpenGLDrawItem {
public:
    using OpenGLDrawItem::OpenGLDrawItem;
    using Vertex = T;
    auto vertices() -> QVector<Vertex>& { return m_vertices; }
    const QVector<Vertex> &vertices() const { return m_vertices; }
    auto geometryChanged(const QRectF &new_, const QRectF &old) -> void;
    virtual auto updateVertexOnGeometryChanged() const -> bool { return false; }
private:
    virtual auto initializeVertex(Vertex *vertex) const -> void;
    virtual auto updateVertex(Vertex *vertex) -> void;
    auto attributeSet() const -> const AttrSet& final { return T::info(); }
    auto createGeometry() const -> QSGGeometry* final;
    auto updateGeometry(QSGGeometry *geometry) -> QSGGeometry* final;
    QVector<Vertex> m_vertices;
};

template<class T>
auto VertexDrawItem<T>::geometryChanged(const QRectF &new_,
                                               const QRectF &old) -> void
{
    OpenGLDrawItem::geometryChanged(new_, old);
    if (updateVertexOnGeometryChanged())
        reserve(UpdateGeometry);
}

template<class T>
auto VertexDrawItem<T>::initializeVertex(Vertex *) const -> void { }

template<class T>
auto VertexDrawItem<T>::updateVertex(Vertex *vertex) -> void
{ memcpy(vertex, m_vertices.data(), sizeof(Vertex)*vertexCount()); }

template<class T>
auto VertexDrawItem<T>::createGeometry() const -> QSGGeometry*
{
    auto geometry = new QSGGeometry(attributeSet(), vertexCount());
    initializeVertex(static_cast<T*>(geometry->vertexData()));
    return geometry;
}

template<class T>
auto VertexDrawItem<T>::updateGeometry(QSGGeometry *geometry) -> QSGGeometry*
{
    geometry->allocate(vertexCount());
    updateVertex(static_cast<T*>(geometry->vertexData()));
    return geometry;
}

/******************************************************************************/

template<class T>
class ShaderRenderItem : public VertexDrawItem<T> {
public:
    using Type = QSGMaterialType;
    using VertexDrawItem<T>::VertexDrawItem;
    struct ShaderData {
        virtual ~ShaderData() = default;
    };
    struct ShaderIface {
        static QOpenGLFunctions *func() { return VertexDrawItem<T>::func(); }
        virtual ~ShaderIface() = default;
        QByteArray fragmentShader, vertexShader;
        QList<QByteArray> attributes;
        static auto matrixName() -> const char* { return "qt_Matrix"; }
        static auto opacityName() -> const char* { return "qt_Opacity"; }
    private:
        virtual auto resolve(QOpenGLShaderProgram *prog) -> void = 0;
        virtual auto update(QOpenGLShaderProgram *prog,
                            const ShaderData *data) -> void = 0;
        friend class ShaderRenderItem;
    };
    virtual auto isOpaque() const -> bool { return false; }
private:
    virtual auto type() const -> Type* = 0;
    virtual auto createShader() const -> ShaderIface* = 0;
    virtual auto createData() const -> ShaderData* = 0;
    virtual auto updateData(ShaderData *data) -> void = 0;
private:
    struct Material : public QSGMaterial {
        Material(const ShaderRenderItem *item);
        QSGMaterialType *type() const final { return m_item->type(); }
        QSGMaterialShader *createShader() const final;
        ShaderData *data() { return m_data; }
        const ShaderData *data() const { return m_data; }
    private:
        const ShaderRenderItem *m_item = nullptr;
        ShaderData *m_data = nullptr;
    };
    struct Shader : public QSGMaterialShader {
        ShaderIface *m_iface = nullptr;
        QVector<const char*> m_attributes;
        Shader(ShaderIface *iface);
        ~Shader() { delete m_iface; }
        auto vertexShader() const -> const char* final;
        auto fragmentShader() const -> const char* final;
        auto attributeNames() const -> const char *const* final;
        auto initialize() -> void final;
        auto updateState(const RenderState &state,
                         QSGMaterial *new_, QSGMaterial *) -> void final;
    private:
        int loc_matrix = -1, loc_opacity = -1;
    };
    auto createMaterial() const -> QSGMaterial* final;
    auto updateMaterial(QSGMaterial *material) -> QSGMaterial* final;
};

template<class T>
ShaderRenderItem<T>::Material::Material(const ShaderRenderItem *item)
    : m_item(item)
{
    setFlag(Blending, !item->isOpaque());
    m_data = m_item->createData();
}

template<class T>
auto ShaderRenderItem<T>::Material::createShader() const -> QSGMaterialShader*
{ return new Shader(m_item->createShader()); }

template<class T>
ShaderRenderItem<T>::Shader::Shader(ShaderIface *iface)
    : m_iface(iface)
{
    m_attributes.resize(m_iface->attributes.size()+1);
    for (int i=0; i<m_iface->attributes.size(); ++i)
        m_attributes[i] = m_iface->attributes[i].constData();
    m_attributes.last() = nullptr;
}

template<class T>
auto ShaderRenderItem<T>::Shader::vertexShader() const -> const char*
{ return m_iface->vertexShader.constData(); }

template<class T>
auto ShaderRenderItem<T>::Shader::fragmentShader() const -> const char*
{ return m_iface->fragmentShader.constData(); }

template<class T>
auto ShaderRenderItem<T>::Shader::attributeNames() const -> const char *const*
{ return m_attributes.data(); }

template<class T>
auto ShaderRenderItem<T>::Shader::initialize() -> void
{
    auto prog = program();
    loc_matrix = prog->uniformLocation(m_iface->matrixName());
    loc_opacity = prog->uniformLocation(m_iface->opacityName());
    m_iface->resolve(prog);
}

template<class T>
auto ShaderRenderItem<T>::Shader::updateState(const RenderState &state,
                                              QSGMaterial *new_,
                                              QSGMaterial *) -> void
{
    auto prog = program();
    if (state.isMatrixDirty())
        prog->setUniformValue(loc_matrix, state.combinedMatrix());
    if (state.isOpacityDirty())
        prog->setUniformValue(loc_opacity, state.opacity());
    auto m = static_cast<Material*>(new_);
    m_iface->update(prog, m->data());
}

template<class T>
auto ShaderRenderItem<T>::createMaterial() const -> QSGMaterial*
{ return new Material(this); }

template<class T>
auto ShaderRenderItem<T>::updateMaterial(QSGMaterial *material) -> QSGMaterial*
{
    auto m = static_cast<Material*>(material);
    updateData(m->data());
    return m;
}

#endif // OPENGLDRAWITEM_HPP
