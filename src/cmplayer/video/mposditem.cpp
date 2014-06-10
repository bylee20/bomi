#include "mposditem.hpp"
#include "mposdbitmap.hpp"
#include "videoimagepool.hpp"
#include "misc/dataevent.hpp"
#include "tmp/static_op.hpp"
#include "opengl/openglvertex.hpp"
#include "opengl/opengltexturebinder.hpp"
#include "opengl/openglframebufferobject.hpp"

enum Attr {AttrPosition, AttrTexCoord, AttrColor};

struct MpOsdItem::Data {
    MpOsdItem *p = nullptr;
    QSize atlasSize = {0, 0};
    MpOsdBitmap::Format format = MpOsdBitmap::Ass;
    int loc_atlas = 0, loc_matrix = 0;
    QOpenGLShaderProgram *shader = nullptr;
    OpenGLFramebufferObject *fbo = nullptr;
    OpenGLTexture2D atlas;
    OpenGLTextureTransferInfo transfer;
    QMatrix4x4 vMatrix;
    QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
    int prevVboSize = 0;
    bool visible = false;
    auto build(MpOsdBitmap::Format inFormat) -> void
    {
        if (!_Change(format, inFormat) && shader)
            return;
        _Renew(shader);
        atlasSize = {};
        const auto tformat = format & MpOsdBitmap::Rgba ? OGL::BGRA
                                                        : OGL::OneComponent;
        transfer = OpenGLTextureTransferInfo::get(tformat);

        QByteArray frag;
        if (format == MpOsdBitmap::Ass) {
            frag = R"(
                uniform sampler2D atlas;
                varying vec4 c;
                varying vec2 texCoord;
                void main() {
                    float a = c.a*texture2D(atlas, texCoord).r;
                    gl_FragColor = vec4(c.rgb*a, a);
                }
            )";
        } else {
            frag = R"(
                uniform sampler2D atlas;
                varying vec2 texCoord;
                void main() {
                    gl_FragColor = texture2D(atlas, texCoord);
                }
            )";
        }
        shader->removeAllShaders();
        shader->addShaderFromSourceCode(QOpenGLShader::Fragment, frag);
        shader->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
            uniform mat4 matrix;
            varying vec4 c;
            varying vec2 texCoord;
            attribute vec4 aPosition;
            attribute vec2 aTexCoord;
            attribute vec4 aColor;
            void main() {
                c = aColor.abgr;
                texCoord = aTexCoord;
                gl_Position = matrix*aPosition;
            }
        )");
        shader->bindAttributeLocation("aTexCoord", AttrTexCoord);
        shader->bindAttributeLocation("aPosition", AttrPosition);
        shader->bindAttributeLocation("aColor", AttrColor);
        if (!shader->link())
            return;
        loc_atlas = shader->uniformLocation("atlas");
        loc_matrix = shader->uniformLocation("matrix");
        shader->bind();
        shader->setUniformValue(loc_atlas, 0);
        shader->release();
    }

    auto initializeAtlas(const MpOsdBitmap &osd) -> void
    {
        using tmp::aligned;
        static const int max = OGL::maximumTextureSize();
        if (osd.sheet().width() > atlasSize.width() || osd.sheet().height() > atlasSize.height()) {
            if (osd.sheet().width() > atlasSize.width())
                atlasSize.rwidth() = qMin(aligned<4>(osd.sheet().width()*1.5), max);
            if (osd.sheet().height() > atlasSize.height())
                atlasSize.rheight() = qMin(aligned<4>(osd.sheet().height()*1.5), max);
            atlas.initialize(atlasSize, transfer);
        }
    }

    auto draw(OpenGLFramebufferObject *fbo, const Cache &cache) -> void
    {
        Q_ASSERT(fbo->isValid());

        vMatrix.setToIdentity();
        vMatrix.ortho(0, fbo->width(), 0, fbo->height(), -1, 1);

        fbo->bind();
        glViewport(0, 0, fbo->width(), fbo->height());
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        const MpOsdBitmap &osd = *cache;
        Q_ASSERT(fbo->size() == osd.renderSize());
        glActiveTexture(GL_TEXTURE0);
        OpenGLTextureBinder<OGL::Target2D> binder(&atlas);

        build(osd.format());
        initializeAtlas(osd);
        Q_ASSERT(shader->isLinked());

        using Vertex = OGL::TextureColorVertex;
        const int num = osd.count();
        vbo.bind();
        if (prevVboSize < static_cast<int>(num*6))
            vbo.allocate((prevVboSize = num*6*1.2)*sizeof(Vertex));
        auto vertex = static_cast<Vertex*>(vbo.map(QOpenGLBuffer::WriteOnly));
        for (int i = 0; i < num; ++i) {
            auto &part = osd.part(i);
            atlas.upload(part.map().x(), part.map().y(),
                         part.strideAsPixel(), part.height(), osd.data(i));
            QPointF tp = part.map();
            tp.rx() /= atlas.width();
            tp.ry() /= atlas.height();
            QSizeF ts = part.size();
            ts.rwidth() /= atlas.width();
            ts.rheight() /= atlas.height();
            const auto &pos = part.display();
            const QRectF tex(tp, ts);
            vertex = OGL::CoordAttr::fillTriangles(vertex,
                        &Vertex::position, pos.topLeft(),
                                           pos.bottomRight() + QPointF(1, 1),
                        &Vertex::texCoord, tex.topLeft(), tex.bottomRight(),
                        [&](Vertex *const it) { it->color.set(part.color()); });
        }
        vbo.unmap();

        shader->bind();

        SET_ATTR_COORD(shader, AttrPosition, Vertex, position);
        SET_ATTR_COORD(shader, AttrTexCoord, Vertex, texCoord);
        SET_ATTR_COLOR(shader, AttrColor, Vertex, color);
        shader->enableAttributeArray(AttrPosition);
        shader->enableAttributeArray(AttrTexCoord);
        shader->enableAttributeArray(AttrColor);
        shader->setUniformValue(loc_matrix, vMatrix);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_TRIANGLES, 0, 6*num);
        glDisable(GL_BLEND);

        shader->disableAttributeArray(AttrPosition);
        shader->disableAttributeArray(AttrTexCoord);
        shader->disableAttributeArray(AttrColor);
        shader->release();
        vbo.release();
        fbo->release();
    }
};

MpOsdItem::MpOsdItem()
    : d(new Data)
{
    d->p = this;
}

MpOsdItem::~MpOsdItem() {
    delete d;
}

auto MpOsdItem::initialize() -> void
{
    d->atlas.create(OGL::Linear, OGL::ClampToEdge);
    d->vbo.create();
    d->vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
}

auto MpOsdItem::finalize() -> void
{
    d->atlas.destroy();
    _Delete(d->shader);
    d->vbo.destroy();
}

auto MpOsdItem::texture() const -> const OpenGLTexture2D&
{
    Q_ASSERT(d->fbo);
    return d->fbo->texture();
}

auto MpOsdItem:: draw(const Cache &cache) -> bool
{
    if (!cache || cache->renderSize().isEmpty())
        return d->visible = false;
    if (!d->fbo || d->fbo->size() != cache->renderSize())
        _Renew(d->fbo, cache->renderSize(), OGL::RGBA8_UNorm);
    d->draw(d->fbo, cache);
    return d->visible = true;
}

auto MpOsdItem::isVisible() const noexcept -> bool
{
    return d->visible;
}
