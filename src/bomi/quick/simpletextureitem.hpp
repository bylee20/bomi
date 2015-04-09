#ifndef SIMPLETEXTUREITEM_HPP
#define SIMPLETEXTUREITEM_HPP

#include "opengldrawitem.hpp"
#include "opengl/openglvertex.hpp"

class OpenGLTexture2D;

class SimpleTextureItem : public ShaderRenderItem<OGL::TextureVertex> {
public:
    SimpleTextureItem(QQuickItem *parent = nullptr);
    ~SimpleTextureItem();
    auto setTexture(const OpenGLTexture2D &texture) -> void;
    OpenGLTexture2D &texture() { return *m_texture; }
    const OpenGLTexture2D &texture() const { return *m_texture; }
    auto drawingMode() const -> GLenum override { return GL_TRIANGLE_STRIP; }
    Type *type() const override { static Type type; return &type; }
    auto vertexCount() const -> int override { return 4; }
protected:
    virtual auto updateTexture(OpenGLTexture2D *texture) -> void { Q_UNUSED(texture); }
    auto updateVertex(Vertex *vertex) -> void override;
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
    OpenGLTexture2D *m_texture = nullptr;
};

#endif // SIMPLETEXTUREITEM_HPP
