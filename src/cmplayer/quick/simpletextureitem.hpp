#ifndef SIMPLETEXTUREITEM_HPP
#define SIMPLETEXTUREITEM_HPP

#include "opengldrawitem.hpp"
#include "opengl/openglmisc.hpp"
#include "opengl/openglvertex.hpp"

class SimpleTextureItem : public ShaderRenderItem<OGL::TextureVertex> {
    Q_OBJECT
public:
    SimpleTextureItem(QQuickItem *parent = nullptr);
    auto setTexture(const OpenGLTexture2D &texture) -> void {
        m_texture = texture; reserve(UpdateMaterial);
    }
    OpenGLTexture2D &texture() { return m_texture; }
    const OpenGLTexture2D &texture() const { return m_texture; }
    auto drawingMode() const -> GLenum override { return GL_TRIANGLE_STRIP; }
    Type *type() const override { static Type type; return &type; }
    auto vertexCount() const -> int override { return 4; }
protected:
    virtual auto updateTexture(OpenGLTexture2D *texture) -> void { Q_UNUSED(texture); }
private:
    auto initializeVertex(Vertex *vertex) const -> void override {
        if (drawingMode() == GL_TRIANGLES)
            Vertex::fillAsTriangles(vertex, {0, 0}, {0, 0}, {0, 0}, {1, 1});
        else if (drawingMode() == GL_TRIANGLE_STRIP)
            Vertex::fillAsTriangleStrip(vertex, {0, 0}, {0, 0}, {0, 0}, {1, 1});
    }
    ShaderIface *createShader() const override;
    ShaderData *createData() const override;
    /** If updateData() is overridden,
     *  updateTexture() should be called in there.*/
    auto updateData(ShaderData *data) -> void override;
    OpenGLTexture2D m_texture;
};

#endif // SIMPLETEXTUREITEM_HPP
