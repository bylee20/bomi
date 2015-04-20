#include "mpvosdrenderer.hpp"
#include "opengl/openglvertex.hpp"
#include "opengl/opengltexturebinder.hpp"
#include "tmp/static_op.hpp"
#include <QOpenGLBuffer>
extern "C" {
#include <sub/osd.h>
}

enum Attr {AttrPosition, AttrTexCoord, AttrColor};
using Vertex = OGL::TextureColorVertex;

struct PartInfo {
    QPoint map = {0, 0};
    quint32 color = 0;
    int strideAsPixel = 0;
};

struct MpvOsdRenderer::Data {
    MpvOsdRenderer *p = nullptr;
    struct {
        int id = -1;
        OpenGLFramebufferObject *fbo = nullptr;
        QSize size;
    } last;

    bool clear = false;
    QSize atlasSize = {0, 0};
    int format = SUBBITMAP_LIBASS;
    int loc_atlas = 0, loc_matrix = 0;
    QOpenGLShaderProgram *shader = nullptr;
    OpenGLFramebufferObject *fbo = nullptr;
    OpenGLTexture2D atlas;
    OpenGLTextureTransferInfo transfer;
    QMatrix4x4 vMatrix;
    QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
    int prevVboSize = 0;
    QOpenGLFunctions *func = nullptr;
    QVector<PartInfo> parts;

    auto build(int inFormat) -> void
    {
        if (!_Change(format, inFormat) && shader)
            return;
        _Renew(shader);
        atlasSize = {};
        const auto tformat = format & SUBBITMAP_RGBA ? OGL::BGRA
                                                     : OGL::OneComponent;
        transfer = OpenGLTextureTransferInfo::get(tformat);
        QByteArray frag;
        if (format == SUBBITMAP_LIBASS) {
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
        shader->link();
        Q_ASSERT(shader->isLinked());

        loc_atlas = shader->uniformLocation("atlas");
        loc_matrix = shader->uniformLocation("matrix");
        shader->bind();
        shader->setUniformValue(loc_atlas, 0);
        shader->release();
    }
    auto initializeAtlas(const sub_bitmaps *imgs) -> void
    {
        using tmp::aligned;
        static const int max = OGL::maximumTextureSize();
        if (parts.size() < imgs->num_parts)
            _Expand(parts, imgs->num_parts);
        static constexpr int shifts[] = { 0, 0, 2, 2 };
        const int shift = shifts[imgs->format];
        QSize sheet{0, 0}; QPoint map{0, 0};
        int lineHeight = 0;

        for (int i=0; i<imgs->num_parts; ++i) {
            auto &img = imgs->parts[i];
            auto &part = parts[i];
            if (imgs->format == SUBBITMAP_LIBASS) {
                const quint32 color = img.libass.color;
                part.color = (color & 0xffffff00) | (0xff - (color & 0xff));
            }
            part.strideAsPixel = (img.stride >> shift);
            if (map.x() + part.strideAsPixel >= max) {
                map.rx() = 0; map.ry() += lineHeight;
                lineHeight = 0;
            }
            part.map = map;
            if (img.h > lineHeight)
                lineHeight = img.h;
            if (map.x() + part.strideAsPixel > sheet.width())
                sheet.rwidth() = map.x() + part.strideAsPixel;
            map.rx() += img.w;
        }
        sheet.rheight() = map.y() + lineHeight;

        if (sheet.width() > atlasSize.width() || sheet.height() > atlasSize.height()) {
            if (sheet.width() > atlasSize.width())
                atlasSize.rwidth() = qMin(aligned<4>(sheet.width()*1.5), max);
            if (sheet.height() > atlasSize.height())
                atlasSize.rheight() = qMin(aligned<4>(sheet.height()*1.5), max);
            atlas.initialize(atlasSize, transfer);
        }
    }
};

MpvOsdRenderer::MpvOsdRenderer()
    : d(new Data)
{
    d->p = this;
}
MpvOsdRenderer::~MpvOsdRenderer() {
    delete d;
}
auto MpvOsdRenderer::initialize() -> void
{
    d->atlas.create(OGL::Linear, OGL::ClampToEdge);
    d->vbo.create();
    d->vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    d->func = OGL::func();
}
auto MpvOsdRenderer::finalize() -> void
{
    d->atlas.destroy();
    _Delete(d->shader);
    d->vbo.destroy();
}

auto MpvOsdRenderer::prepare(OpenGLFramebufferObject *fbo) -> void
{
    d->clear = true;
    d->fbo = fbo;
}

SIA alignment(int stride) -> int
{
    if (!tmp::remainder<8>(stride))
        return 8;
    if (!tmp::remainder<4>(stride))
        return 4;
    if (!tmp::remainder<2>(stride))
        return 2;
    return 1;
}

auto MpvOsdRenderer::draw(const sub_bitmaps *imgs) -> void
{
    if (!d->fbo || !d->fbo->isValid())
        return;
    const int num = imgs->num_parts;
    if (num <= 0 || imgs->format == SUBBITMAP_EMPTY)
        return;
    d->clear = false;
    if (d->last.id == imgs->change_id
            && d->last.fbo == d->fbo && d->last.size == d->fbo->size())
        return;

    d->vbo.bind();

    d->func->glActiveTexture(GL_TEXTURE0);
    OpenGLTextureBinder<OGL::Target2D> binder(&d->atlas);

    if (_Change(d->last.id, imgs->change_id)) {
        d->build(imgs->format);
        d->initializeAtlas(imgs);
        if (d->prevVboSize < static_cast<int>(num*6))
            d->vbo.allocate((d->prevVboSize = num*6*1.2)*sizeof(Vertex));
        auto vertex = static_cast<Vertex*>(d->vbo.map(QOpenGLBuffer::WriteOnly));
        for (int i = 0; i < num; ++i) {
            const auto &part = d->parts[i];
            const auto &img = imgs->parts[i];
            Q_ASSERT(part.map.x() + part.strideAsPixel <= d->atlas.width());
            Q_ASSERT(part.map.y() + img.h <= d->atlas.height());

            glPixelStorei(GL_UNPACK_ALIGNMENT, alignment(img.stride));
            glPixelStorei(GL_UNPACK_ROW_LENGTH, part.strideAsPixel);

            d->atlas.upload(part.map.x(), part.map.y(), img.w, img.h, img.bitmap);

            QPointF tp = part.map; QSizeF ts(img.w, img.h);
            tp.rx() /= d->atlas.width();
            tp.ry() /= d->atlas.height();
            ts.rwidth() /= d->atlas.width();
            ts.rheight() /= d->atlas.height();
            const QRectF pos(img.x, img.y, img.dw, img.dh), tex(tp, ts);
            vertex = OGL::CoordAttr::fillTriangles(vertex,
                &Vertex::position, pos.topLeft(), pos.bottomRight(),
                &Vertex::texCoord, tex.topLeft(), tex.bottomRight(),
                [&](Vertex *const it) { it->color.set(part.color); });

            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        }
        d->vbo.unmap();
    }

    d->vMatrix.setToIdentity();
    d->vMatrix.ortho(0, d->fbo->width(), 0, d->fbo->height(), -1, 1);

    d->fbo->bind();
    d->last.fbo = d->fbo;
    d->last.size = d->fbo->size();
    glViewport(0, 0, d->fbo->width(), d->fbo->height());
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    d->shader->bind();
    SET_ATTR_COORD(d->shader, AttrPosition, Vertex, position);
    SET_ATTR_COORD(d->shader, AttrTexCoord, Vertex, texCoord);
    SET_ATTR_COLOR(d->shader, AttrColor, Vertex, color);
    d->shader->enableAttributeArray(AttrPosition);
    d->shader->enableAttributeArray(AttrTexCoord);
    d->shader->enableAttributeArray(AttrColor);
    d->shader->setUniformValue(d->loc_matrix, d->vMatrix);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 6*num);
    glDisable(GL_BLEND);
    d->shader->disableAttributeArray(AttrPosition);
    d->shader->disableAttributeArray(AttrTexCoord);
    d->shader->disableAttributeArray(AttrColor);
    d->shader->release();
    d->vbo.release();
    d->fbo->release();
}

auto MpvOsdRenderer::end() -> void
{
    if (d->clear) {
        d->fbo->bind();
        glViewport(0, 0, d->fbo->width(), d->fbo->height());
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        d->fbo->release();
    }
}

auto MpvOsdRenderer::callback(void *ctx, sub_bitmaps *imgs) -> void
{
    static_cast<MpvOsdRenderer*>(ctx)->draw(imgs);
}

void osd_draw(struct osd_state *osd, struct mp_osd_res res,
              double video_pts, int draw_flags,
              const bool formats[SUBBITMAP_COUNT],
              void (*cb)(void *ctx, struct sub_bitmaps *imgs), void *cb_ctx);
