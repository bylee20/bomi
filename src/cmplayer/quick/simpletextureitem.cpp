#include "simpletextureitem.hpp"

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
                const SimpleTextureItem::ShaderData *data) override {
        auto d = static_cast<const SimpleTextureData*>(data);
        if (d->texture->id() != GL_NONE && !d->texture->isEmpty()) {
            prog->setUniformValue(loc_tex, 0);
            glActiveTexture(GL_TEXTURE0);
            d->texture->bind();
        }
    }
private:
    int loc_tex = -1;
};

SimpleTextureItem::SimpleTextureItem(QQuickItem *parent)
    : ShaderRenderItem<OGL::TextureVertex>(parent)
{

}

SimpleTextureItem::ShaderIface *SimpleTextureItem::createShader() const {
    return new SimpleTextureShader;
}

SimpleTextureItem::ShaderData *SimpleTextureItem::createData() const {
    auto data = new SimpleTextureData;
    data->texture = &m_texture;
    return data;
}

void SimpleTextureItem::updateData(ShaderData *data) {
    Q_ASSERT(static_cast<SimpleTextureData*>(data)->texture == &m_texture);
    updateTexture(&m_texture);
}
