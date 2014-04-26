#include "mposditem.hpp"
#include "opengl/openglvertex.hpp"
#include "mposdbitmap.hpp"
#include "dataevent.hpp"
#include "opengl/openglcompat.hpp"

enum Attr {AttrPosition, AttrTexCoord, AttrColor};

struct MpOsdItem::Data {
    MpOsdItem *p = nullptr;
    MpOsdBitmap osd;
    QSize imageSize = {0, 0}, atlasSize = {0, 0};
    bool show = false;
    MpOsdBitmap::Format format = MpOsdBitmap::Ass;
    GLenum srcFactor = GL_SRC_ALPHA;
    int loc_atlas = 0, loc_matrix = 0;
    QOpenGLShaderProgram *shader = nullptr;
    OpenGLTexture2D atlas;
    OpenGLTextureTransferInfo transfer;
    QMatrix4x4 vMatrix;
    QOpenGLBuffer vbo{QOpenGLBuffer::VertexBuffer};
    int prevVboSize = 0;
    void build(MpOsdBitmap::Format inFormat) {
        if (!_Change(format, inFormat) && shader)
            return;
        _Renew(shader);
        srcFactor = (format & MpOsdBitmap::PaMask) ? GL_ONE : GL_SRC_ALPHA;
        atlasSize = {};
        const auto tformat = format & MpOsdBitmap::Rgba ? OGL::BGRA
                                                        : OGL::OneComponent;
        transfer = OpenGLCompat::textureTransferInfo(tformat);

        QByteArray frag;
        if (format == MpOsdBitmap::Ass) {
            frag = R"(
                uniform sampler2D atlas;
                varying vec4 c;
                varying vec2 texCoord;
                void main() {
                    vec2 co = vec2(c.a*texture2D(atlas, texCoord).r, 0.0);
                    gl_FragColor = c*co.xxxy + co.yyyx;
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

    void initializeAtlas(const MpOsdBitmap &osd) {
        static const int max = OpenGLCompat::maximumTextureSize();
        if (osd.sheet().width() > atlasSize.width() || osd.sheet().height() > atlasSize.height()) {
            if (osd.sheet().width() > atlasSize.width())
                atlasSize.rwidth() = qMin<int>(_Aligned<4>(osd.sheet().width()*1.5), max);
            if (osd.sheet().height() > atlasSize.height())
                atlasSize.rheight() = qMin<int>(_Aligned<4>(osd.sheet().height()*1.5), max);
            atlas.initialize(atlasSize, transfer);
        }
    }

    void draw(OpenGLFramebufferObject *fbo, const MpOsdBitmap &osd) {
        if (!fbo->isValid())
            return;
        build(osd.format());
        if (!shader->isLinked())
            return;
        glActiveTexture(GL_TEXTURE0);
        OpenGLTextureBinder<OGL::Target2D> binder(&atlas);
        initializeAtlas(osd);

        Q_ASSERT(fbo->size() == osd.renderSize());
        vMatrix.setToIdentity();
        vMatrix.ortho(0, fbo->width(), 0, fbo->height(), -1, 1);

        fbo->bind();
        glViewport(0, 0, fbo->width(), fbo->height());
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

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
                        &Vertex::position, pos.topLeft(), pos.bottomRight(),
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
        glBlendFunc(srcFactor, GL_ONE_MINUS_SRC_ALPHA);
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

MpOsdItem::MpOsdItem(QQuickItem *parent)
: SimpleFboItem(parent), d(new Data) {
    d->p = this;
}

MpOsdItem::~MpOsdItem() {
    delete d;
}

void MpOsdItem::initializeGL() {
    SimpleFboItem::initializeGL();
    d->atlas.create(OGL::Linear, OGL::ClampToEdge);
    d->vbo.create();
    d->vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
}

void MpOsdItem::finalizeGL() {
    SimpleFboItem::finalizeGL();
    d->atlas.destroy();
    _Delete(d->shader);
    d->vbo.destroy();
}

void MpOsdItem::drawOn(sub_bitmaps *imgs) {
    d->show = true;
    if (!d->osd.needToCopy(imgs))
        return;
    MpOsdBitmap osd;
    if (osd.copy(imgs, d->imageSize))
        _PostEvent(this, EnqueueFrame, osd);
}

void MpOsdItem::drawOn(QImage &frame) {
    if (isVisible())
        d->osd.drawOn(frame);
}

void MpOsdItem::present(bool redraw) {
    if (redraw)
        return;
    if (d->show) {
        _PostEvent(this, Show);
        d->show = false;
    } else
        _PostEvent(this, Hide);
}

void MpOsdItem::customEvent(QEvent *event) {
    QQuickItem::customEvent(event);
    switch ((int)event->type()) {
    case Show:
        setVisible(true);
        update();
        break;
    case Hide:
        setVisible(false);
        break;
    case EnqueueFrame:
        setVisible(true);
        _GetAllData(event, d->osd);
        forceRepaint();
        break;
    default:
        break;
    }
}

QSize MpOsdItem::imageSize() const {
    return d->osd.renderSize();
}

void MpOsdItem::paint(OpenGLFramebufferObject *fbo) {
    d->draw(fbo, d->osd);
}

void MpOsdItem::setImageSize(const QSize &size) {
    d->imageSize = size;
}
