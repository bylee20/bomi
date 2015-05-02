#include "simpletextureitem.hpp"
#include "opengl/opengltexture2d.hpp"

struct SimpleTextureData : public SimpleTextureItem::ShaderData {
    const OpenGLTexture2D *texture = nullptr;
};

struct SimpleTextureShader : public SimpleTextureItem::ShaderIface {
    SimpleTextureShader() {
        vertexShader = R"(
            uniform mat4 qt_Matrix;
            attribute vec4 aPosition;
            attribute vec2 aTexCoord;
            varying vec2 texCoord;
            void main() {
                texCoord = aTexCoord;
                gl_Position = qt_Matrix * aPosition;
            }
        )";
        fragmentShader = R"(
            varying vec2 texCoord;
            uniform float qt_Opacity;
            uniform sampler2D tex;
            void main() {
                gl_FragColor = texture2D(tex, texCoord) * qt_Opacity;
            }
        )";
        attributes << "aPosition" << "aTexCoord";
    }
    void resolve(QOpenGLShaderProgram *prog) override {
        Q_ASSERT(prog->isLinked());
        loc_tex = prog->uniformLocation("tex");
    }
    void update(QOpenGLShaderProgram *prog,
                SimpleTextureItem::ShaderData *data) override {
        auto d = static_cast<const SimpleTextureData*>(data);
        if (d->texture->id() != GL_NONE && !d->texture->isEmpty()) {
            prog->setUniformValue(loc_tex, 0);
            func()->glActiveTexture(GL_TEXTURE0);
            d->texture->bind();
        }
    }
private:
    int loc_tex = -1;
};

SimpleTextureItem::SimpleTextureItem(QQuickItem *parent)
    : ShaderRenderItem<OGL::TextureVertex>(parent)
{
    m_texture = new OpenGLTexture2D;
}

SimpleTextureItem::~SimpleTextureItem()
{
    delete m_texture;
}

auto SimpleTextureItem::createShader() const -> SimpleTextureItem::ShaderIface*
{
    return new SimpleTextureShader;
}

auto SimpleTextureItem::createData() const -> SimpleTextureItem::ShaderData*
{
    auto data = new SimpleTextureData;
    data->texture = m_texture;
    return data;
}

auto SimpleTextureItem::updateData(ShaderData *data) -> void
{
    Q_ASSERT(static_cast<SimpleTextureData*>(data)->texture == m_texture);
    Q_UNUSED(data);
    updateTexture(m_texture);
}

auto SimpleTextureItem::updateVertex(Vertex *vertex) -> void
{
    OGL::CoordAttr::fillTriangleStrip(vertex, &Vertex::position,
                                      {0, 0}, {width(), height()});
}

auto SimpleTextureItem::setTexture(const OpenGLTexture2D &texture) -> void
{
    *m_texture = texture;
    reserve(UpdateMaterial);
}
