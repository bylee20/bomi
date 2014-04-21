#ifndef SIMPLETEXTUREITEM_HPP
#define SIMPLETEXTUREITEM_HPP

#include "opengldrawitem.hpp"
#include "openglmisc.hpp"
#include "openglvertex.hpp"

class SimpleTextureItem : public ShaderRenderItem<OGL::TextureVertex> {
    Q_OBJECT
public:
    SimpleTextureItem(QQuickItem *parent = nullptr);
    void setTexture(const OpenGLTexture2D &texture) {
        m_texture = texture; reserve(UpdateMaterial);
    }
    OpenGLTexture2D &texture() { return m_texture; }
    const OpenGLTexture2D &texture() const { return m_texture; }
    GLenum drawingMode() const override { return GL_TRIANGLE_STRIP; }
    Type *type() const override { static Type type; return &type; }
    int vertexCount() const override { return 4; }
protected:
    virtual void updateTexture(OpenGLTexture2D *texture) { Q_UNUSED(texture); }
private:
    void initializeVertex(Vertex *vertex) const override {
        if (drawingMode() == GL_TRIANGLES)
            Vertex::fillAsTriangles(vertex, {0, 0}, {0, 0}, {0, 0}, {1, 1});
        else if (drawingMode() == GL_TRIANGLE_STRIP)
            Vertex::fillAsTriangleStrip(vertex, {0, 0}, {0, 0}, {0, 0}, {1, 1});
    }
    ShaderIface *createShader() const override;
    ShaderData *createData() const override;
    /** If updateData() is overridden,
     *  updateTexture() should be called in there.*/
    void updateData(ShaderData *data) override;
    OpenGLTexture2D m_texture;
};

#endif // SIMPLETEXTUREITEM_HPP
