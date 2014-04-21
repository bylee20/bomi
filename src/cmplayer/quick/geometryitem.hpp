#ifndef GEOMETRYITEM_HPP
#define GEOMETRYITEM_HPP

#include "stdafx.hpp"

class GeometryItem : public QQuickItem {
    Q_OBJECT
public:
    GeometryItem(QQuickItem *parent = nullptr): QQuickItem(parent) {}
    void setGeometry(const QPointF &pos, const QSizeF &size) {setPosition(pos); setSize(size);}
    void setGeometry(const QRectF &rect) {setPosition(rect.topLeft()); setSize(rect.size());}
    QSizeF size() const { return {width(), height()}; }
    QRectF geometry() const { return {position(), size()}; }
    QRectF rect() const { return {0.0, 0.0, width(), height()}; }
protected:
    void geometryChanged(const QRectF &new_, const QRectF &old) override {
        QQuickItem::geometryChanged(new_, old);
    }
};



//template<class State>
//class OpenGLRendererMaterial : public QSGMaterial {
//public:
//private:
//    QSGMaterialType *type() const override final { return &m_type; }
//    QSGMaterialShader *createShader() const override final;
//    mutable QSGMaterialType m_type;
//};

//template<class State>
//class OpenGLRendererShader : public QSGMaterialShader {
//public:
//    virtual QList<QByteArray> attributes() const = 0;
//    virtual QByteArray vertexShaderSource() const = 0;
//    virtual QByteArray fragmentShaderSource() const = 0;
//    virtual void resolve() { }
//    virtual void update() { }
//protected:
//    void initialize() {
//        QSGMaterialShader::initialize();
//        auto prog = program();
//        m_loc_matrix = prog->uniformLocation("qt_Matrix");
//        m_loc_opacity = prog->uniformLocation("qt_Opacity");
//        resolve(program());
//    }
//    void updateState(const RenderState &state, QSGMaterial *new_, QSGMaterial *old) {
//        auto prog = program();
//        if (state.isMatrixDirty())
//            prog->setUniformValue(m_loc_matrix, state.combinedMatrix());
//        if (state.isOpacityDirty())
//            prog->setUniformValue(m_loc_opacity, state.opacity());
//        auto mate = static_cast<OpenGLRendererMaterial*>(new_);
//        auto prev = static_cast<OpenGLRendererMaterial*>(old);
//        update(mate, prev);
//    }

//private:
//    friend class OpenGLRendererMaterial;
//    const char *const *attributeNames() const override final {
//        if (m_attributes.isEmpty()) {
//            m_attrList = material->attributes();
//            m_attributes.resize(m_attrList.size());
//            for (int i = 0; i < m_attrList.size(); ++i)
//                m_attributes[i] = m_attrList[i].data();
//        }
//        return m_attributes.constData();
//    }
//    const char *fragmentShader() const override final {
//        if (m_fragmentShader.isEmpty())
//            m_fragmentShader = fragmentShaderSource();
//        return m_fragmentShader.data();
//    }
//    const char *vertexShader() const override final {
//        if (m_vertexShader.isEmpty())
//            m_vertexShader = vertexShaderSource();
//        return m_vertexShader.data();
//    }
//    QList<QByteArray> m_attrList;
//    QVector<const char*> m_attributes;
//    QByteArray m_fragmentShader, m_vertexShader;
//    int m_loc_matrix = -1, m_loc_opacity = -1;
//};

//inline QSGMaterialShader *OpenGLRendererMaterial::createShader() const {
//    return new OpenGLRendererShader(this);
//}


//class TextureRendererShader : public QSGMaterialShader {
//public:
//    TextureRendererShader(const HighQualityTextureItem *item, Interpolator::Category category = Interpolator::None, bool dithering = false, bool rectangle = false);
//    static QOpenGLFunctions *func() { return QOpenGLContext::currentContext()->functions(); }
//    const char *fragmentShader() const override { return m_fragCode.constData(); }
//    const char *vertexShader() const override { return m_vertexCode.constData(); }
//    const char *const *attributeNames() const override;
//    virtual void link(QOpenGLShaderProgram *prog);
//    virtual void bind(QOpenGLShaderProgram *prog);
//    const HighQualityTextureItem *item() const { return m_item; }
//private:
//    void updateState(const RenderState &state, QSGMaterial */*new_*/, QSGMaterial */*old*/) final override;
//    void initialize() final override;
//    const HighQualityTextureItem *m_item = nullptr;
//    Interpolator::Category m_category = Interpolator::None;
//    bool m_dithering = false, m_rectangle = false;
//    int m_lutCount = 0;
//    int loc_lut_int[2] = {-1, -1}, loc_lut_int_mul[2] = {-1, -1};
//    int loc_tex = -1, loc_vMatrix = -1, loc_dxy = -1;
//    int loc_tex_size = -1;
//    int loc_dithering = -1, loc_dithering_quantization = -1, loc_dithering_center = -1, loc_dithering_size = -1;
//    QByteArray m_fragCode, m_vertexCode, m_forLog;
//};






#endif // GEOMETRYITEM_HPP
