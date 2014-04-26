#ifndef OPENGLDRAWITEM_HPP
#define OPENGLDRAWITEM_HPP

#include "geometryitem.hpp"
#include "opengl/openglmisc.hpp"

class OpenGLDrawItem : public GeometryItem {
    Q_OBJECT
public:
    struct ColorVertex { QPointF vertex; QColor color; };
    enum UpdateHint {
        UpdateGeometry = 1,
        UpdateMaterial = 2,
        UpdateAll = UpdateGeometry | UpdateMaterial
    };
    OpenGLDrawItem(QQuickItem *parent = nullptr);
    ~OpenGLDrawItem();
    virtual void rerender() { reserve(UpdateAll); }
    double devicePixelRatio() const { return m_win ? m_win->devicePixelRatio() : 1.0; }
    QByteArray logAt(const char *func) const { return QByteArray(metaObject()->className()) + "::" + func; }
    void polishAndUpdate() { polish(); update(); }
    static QOpenGLFunctions *func() { return context()->functions(); }
protected slots:
    bool isInitialized() const { return m_init; }
    virtual void initializeGL() { }
    virtual void finalizeGL() { }
protected:
    virtual GLenum drawingMode() const { return GL_TRIANGLE_STRIP; }
    virtual const QSGGeometry::AttributeSet &attributeSet() const {
        return QSGGeometry::defaultAttributes_TexturedPoint2D();
    }
    virtual int vertexCount() const { return 0; }
    virtual QSGGeometry *createGeometry() const = 0;
    virtual QSGMaterial *createMaterial() const = 0;
    virtual QSGGeometry *updateGeometry(QSGGeometry *geometry) = 0;
    virtual QSGMaterial *updateMaterial(QSGMaterial *material) = 0;
    virtual void afterUpdate() { }
    static QOpenGLContext *context() { return QOpenGLContext::currentContext(); }
    void reserve(UpdateHint hint, bool update = true)
    { m_updates |= hint; if (update) this->update(); }
    bool isReserved(UpdateHint update) const { return m_updates & update; }
private:
    void tryInitGL() { if (!m_init && context()) { initializeGL(); m_init = true; } }
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) final override;
    QQuickWindow *m_win = nullptr;
    bool m_init = false;
    QSGGeometryNode *m_node = nullptr;
    int m_updates = 0;
};

template <class T>
class VertexDrawItem : public OpenGLDrawItem {
public:
    using OpenGLDrawItem::OpenGLDrawItem;
    using Vertex = T;
    QVector<Vertex> &vertices() { return m_vertices; }
    const QVector<Vertex> &vertices() const { return m_vertices; }
    void geometryChanged(const QRectF &new_, const QRectF &old) {
        OpenGLDrawItem::geometryChanged(new_, old);
        if (updateVertexOnGeometryChanged())
            reserve(UpdateGeometry);
    }
    virtual bool updateVertexOnGeometryChanged() const { return false; }
private:
    virtual void initializeVertex(Vertex *vertex) const { Q_UNUSED(vertex); }
    virtual void updateVertex(Vertex *vertex) {
        memcpy(vertex, m_vertices.data(), sizeof(Vertex)*vertexCount());
    }
    const QSGGeometry::AttributeSet &attributeSet() const final {
        return T::info();
    }
    QSGGeometry *createGeometry() const final {
        auto geometry = new QSGGeometry(attributeSet(), vertexCount());
        initializeVertex(static_cast<T*>(geometry->vertexData()));
        return geometry;
    }
    QSGGeometry *updateGeometry(QSGGeometry *geometry) final {
        geometry->allocate(vertexCount());
        updateVertex(static_cast<T*>(geometry->vertexData()));
        return geometry;
    }

    QVector<Vertex> m_vertices;
};

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
        static const char *matrixName() { return "qt_Matrix"; }
        static const char *opacityName() { return "qt_Opacity"; }
    private:
        virtual void resolve(QOpenGLShaderProgram *prog) = 0;
        virtual void update(QOpenGLShaderProgram *prog, const ShaderData *data) = 0;
        friend class ShaderRenderItem;
    };
private:
    virtual QSGMaterialType *type() const = 0;
    virtual ShaderIface *createShader() const = 0;
    virtual ShaderData *createData() const = 0;
    virtual void updateData(ShaderData *data) = 0;
private:
    struct Material : public QSGMaterial {
        Material(const ShaderRenderItem *item)
            : m_item(item)
        {
            setFlag(Blending);
            m_data = m_item->createData();
        }
        QSGMaterialType *type() const final { return m_item->type(); }
        QSGMaterialShader *createShader() const final {
            return new Shader(m_item->createShader());
        }
        ShaderData *data() { return m_data; }
        const ShaderData *data() const { return m_data; }
    private:
        const ShaderRenderItem *m_item = nullptr;
        ShaderData *m_data = nullptr;
    };
    struct Shader : public QSGMaterialShader {
        ShaderIface *m_iface = nullptr;
        QVector<const char*> m_attributes;
        Shader(ShaderIface *iface): m_iface(iface) {
            m_attributes.resize(m_iface->attributes.size()+1);
            for (int i=0; i<m_iface->attributes.size(); ++i)
                m_attributes[i] = m_iface->attributes[i].constData();
            m_attributes.last() = nullptr;
        }
        ~Shader() { delete m_iface; }
        const char *vertexShader() const final {
            return m_iface->vertexShader.constData();
        }
        const char *fragmentShader() const final {
            return m_iface->fragmentShader.constData();
        }
        const char *const *attributeNames() const final {
            return m_attributes.data();
        }
        void initialize() final {
            auto prog = program();
            loc_matrix = prog->uniformLocation(m_iface->matrixName());
            loc_opacity = prog->uniformLocation(m_iface->opacityName());
            m_iface->resolve(prog);
        }
        void updateState(const RenderState &state, QSGMaterial *new_, QSGMaterial *) {
            auto prog = program();
            if (state.isMatrixDirty())
                prog->setUniformValue(loc_matrix, state.combinedMatrix());
            if (state.isOpacityDirty())
                prog->setUniformValue(loc_opacity, state.opacity());
            auto m = static_cast<Material*>(new_);
            m_iface->update(prog, m->data());
        }
    private:
        int loc_matrix = -1, loc_opacity = -1;
    };
    QSGMaterial *createMaterial() const final { return new Material(this); }
    QSGMaterial *updateMaterial(QSGMaterial *material) final {
        auto m = static_cast<Material*>(material);
        updateData(m->data());
        return m;
    }
};

#endif // OPENGLDRAWITEM_HPP
