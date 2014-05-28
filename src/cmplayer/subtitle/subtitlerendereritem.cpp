#include "subtitlerendereritem.hpp"
#include "subtitlerenderingthread.hpp"
#include "misc/dataevent.hpp"
#include "opengl/opengltexture2d.hpp"
#include "opengl/opengltexturebinder.hpp"

struct SubtitleShaderData : public SubtitleRendererItem::ShaderData {
    const OpenGLTexture2D *texture, *bbox;
    QColor bboxColor;
};

struct SubtitleShader : public SubtitleRendererItem::ShaderIface {
    SubtitleShader()
    {
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
        fragmentShader = (R"(
            uniform sampler2D tex;
            uniform sampler2D bbox;
            uniform vec4 bboxColor;
            varying vec2 texCoord;
            void main() {
                vec4 top = texture2D(tex, texCoord);
                float alpha = texture2D(bbox, texCoord).a*bboxColor.a*(1.0 - top.a);
                gl_FragColor = top + bboxColor*alpha;
            }
        )");
        attributes << "aPosition" << "aTexCoord";
    }
    void resolve(QOpenGLShaderProgram *prog) override {
        loc_tex = prog->uniformLocation("tex");
        loc_bbox = prog->uniformLocation("bbox");
        loc_bboxColor = prog->uniformLocation("bboxColor");
    }
    void update(QOpenGLShaderProgram *prog
                , const SubtitleRendererItem::ShaderData *data) override {
        auto d = static_cast<const SubtitleShaderData*>(data);
        auto f = func();
        d->texture->bind(prog, loc_tex, 0);
        d->bbox->bind(prog, loc_bbox, 1);
        prog->setUniformValue(loc_bboxColor, d->bboxColor);
        f->glActiveTexture(GL_TEXTURE0);
    }
private:
    int loc_tex = -1, loc_bbox = -1, loc_bboxColor = -1;
};

struct SubtitleRendererItem::Data {
    Data(SubtitleRendererItem *p): p(p) {}
    SubtitleRendererItem *p = nullptr;
    QList<SubComp*> loaded;
    QSize imageSize{0, 0};
    SubtitleDrawer drawer;
    int delay = 0, msec = 0;
    bool selecting = false, textChanged = true;
    bool top = false, hidden = false, empty = true;
    double pos = 1.0;
    QMap<QString, int> langMap;
    QMutex mutex;
    QWaitCondition wait;
    RichTextDocument text;
    int language_priority(const SubComp &comp) const {
//        return langMap.value(r->comp->language().id(), -1);
        return langMap.value(comp.language(), -1);
    }
    QVector<quint32> zeros, bboxData;
    SubCompSelection selection{p};
    OpenGLTexture2D bbox;
    double fps() const { return selection.fps(); }
    void updateDrawer() {
        selection.setDrawer(drawer);
        p->reserve(UpdateGeometry);
    }
    void sort() {
        selection.sort([this] (const SubComp &lhs, const SubComp &rhs) {
            return language_priority(lhs) > language_priority(rhs);
        });
    }
    void setMargin(const Margin &margin) {
        drawer.setMargin(margin);
        p->reserve(UpdateGeometry);
    }
    void applySelection() {
        empty = selection.isEmpty();
        emit p->modelsChanged(selection.models());
        if (!empty)
            render(SubCompSelection::Rerender);
        else
            p->setVisible(false);
        textChanged = true;
    }
    void render(int flags) {
        if (!hidden && !empty && msec > 0)
            selection.render(msec - delay, flags);
    }
    void updateVisible() {
        p->setVisible(!hidden && !empty && !imageSize.isEmpty());
    }
};

SubtitleRendererItem::SubtitleRendererItem(QQuickItem *parent)
    : SimpleTextureItem(parent)
    , d(new Data(this))
{
    d->drawer.setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    d->updateDrawer();
}

SubtitleRendererItem::~SubtitleRendererItem() {
    unload();
    delete d;
}

auto SubtitleRendererItem::isTopAligned() const -> bool
{
    return d->top;
}

auto SubtitleRendererItem::render(int ms) -> void
{
    d->msec = ms;
    d->render(SubCompSelection::Tick);
}

auto SubtitleRendererItem::rerender() -> void
{
    d->render(SubCompSelection::Rerender);
}

auto SubtitleRendererItem::createShader() const -> ShaderIface*
{
    return new SubtitleShader;
}

auto SubtitleRendererItem::initializeGL() -> void
{
    SimpleTextureItem::initializeGL();
    texture().create();
    d->bbox.create();
}

auto SubtitleRendererItem::finalizeGL() -> void
{
    SimpleTextureItem::finalizeGL();
    d->bbox.destroy();
    texture().destroy();
}

auto SubtitleRendererItem::text() const -> const RichTextDocument&
{
    if (d->textChanged) {
        d->text.clear();
        d->selection.forImages([this] (const SubCompImage &image) {
            d->text += image.text();
        });
        d->textChanged = false;
    }
    return d->text;
}

auto SubtitleRendererItem::setPriority(const QStringList &priority) -> void
{
    for (int i=0; i< priority.size(); ++i)
        d->langMap[priority[i]] = priority.size()-i;
}

auto SubtitleRendererItem::setStyle(const SubtitleStyle &style) -> void
{
    d->drawer.setStyle(style);
    d->updateDrawer();
}

auto SubtitleRendererItem::draw(const QRectF &rect, QRectF *put) const -> QImage
{
    QImage sub; int gap = 0;
    auto boxes = d->drawer.draw(sub, gap, text(), rect, 1.0);
    if (sub.isNull())
        return QImage();
    if (put)
        *put = {d->drawer.pos(sub.size(), rect), sub.size()};
    if (!boxes.isEmpty()) {
        QImage bg(sub.size(), QImage::Format_ARGB32_Premultiplied);
        bg.fill(0x0);
        QPainter painter(&bg);
        auto bcolor = d->drawer.style().bbox.color;
        bcolor.setAlpha(255);
        for (auto &bbox : boxes)
            painter.fillRect(bbox, bcolor);
        auto p = bg.bits();
        for (int i=0; i<bg.width(); ++i) {
            for (int j=0; j<bg.height(); ++j) {
                *p++ *= d->drawer.style().bbox.color.alphaF();
                *p++ *= d->drawer.style().bbox.color.alphaF();
                *p++ *= d->drawer.style().bbox.color.alphaF();
                *p++ *= d->drawer.style().bbox.color.alphaF();
            }
        }
        painter.drawImage(QPoint(0, 0), sub);
        painter.end();
        sub.swap(bg);
    }
    return sub;
}

auto SubtitleRendererItem::pos() const -> double
{
    return d->pos;
}

auto SubtitleRendererItem::componentsCount() const -> int
{
    return d->loaded.count();
}

auto SubtitleRendererItem::components() const -> QVector<const SubComp *>
{
    QVector<const SubComp*> list;
    list.reserve(d->loaded.size());
    for (auto loaded : d->loaded)
        list.push_back(loaded);
    return list;
}

auto SubtitleRendererItem::setTopAligned(bool top) -> void
{
    if (_Change(d->top, top)) {
        const auto alignment = Qt::AlignHCenter | (d->top ? Qt::AlignTop : Qt::AlignBottom);
        if (d->drawer.alignment() != alignment) {
            d->drawer.setAlignment(alignment);
            d->updateDrawer();
        }
        setPos(1.0 - d->pos);
    }
}

auto SubtitleRendererItem::style() const -> const SubtitleStyle&
{
    return d->drawer.style();
}

auto SubtitleRendererItem::unload() -> void
{
    d->selection.clear();
    qDeleteAll(d->loaded);
    d->loaded.clear();
    setVisible(false);
    d->empty = true;
    d->textChanged = true;
    emit modelsChanged(models());
}

auto SubtitleRendererItem::updateVertex(Vertex *vertex) -> void
{
    const auto dpr = devicePixelRatio();
    const QRectF r(d->drawer. pos(d->imageSize/dpr, rect()), d->imageSize/dpr);
    OGL::CoordAttr::fillTriangleStrip(vertex, &Vertex::position,
                                      r.topLeft(), r.bottomRight());
}

auto SubtitleRendererItem::createData() const -> ShaderData*
{
    auto data = new SubtitleShaderData;
    data->texture = &texture();
    data->bbox = &d->bbox;
    return data;
}

auto SubtitleRendererItem::updateData(ShaderData *sd) -> void
{
    auto data = static_cast<SubtitleShaderData*>(sd);
    updateTexture(&texture());
    data->bboxColor = d->drawer.style().bbox.color;
}

auto SubtitleRendererItem::updateTexture(OpenGLTexture2D *texture) -> void
{
    d->imageSize = {0, 0};
    d->selection.forImages([this] (const SubCompImage &image) {
        if (d->imageSize.width() < image.width())
            d->imageSize.rwidth() = image.width();
        d->imageSize.rheight() += image.height();
    });
    if (!d->imageSize.isEmpty()) {
        const auto len = d->imageSize.width()*d->imageSize.height();
        _Expand(d->zeros, len);
        OpenGLTextureBinder<OGL::Target2D> binder;
        binder.bind(texture);
        texture->initialize(d->imageSize, d->zeros.data());
        binder.bind(&d->bbox);
        d->bbox.initialize(d->imageSize, d->zeros.data());
        int y = 0;
        d->selection.forImages([&] (const SubCompImage &image) {
            const int x = (texture->width() - image.width())*0.5;
            if (!image.isNull()) {
                binder.bind(texture);
                texture->upload(x, y, image.width(), image.height(), image.bits());
                binder.bind(&d->bbox);
                for (auto &bbox : image.boundingBoxes()) {
                    const auto rect = bbox.toRect().translated(x, y);
                    if (_Expand(d->bboxData, rect.width()*rect.height()))
                        d->bboxData.fill(_Max<quint32>());
                    d->bbox.upload(rect, d->bboxData.data());
                }
            }
            y += image.height();
        });
        reserve(UpdateGeometry, false);
    }
}

auto SubtitleRendererItem::afterUpdate() -> void
{
    d->updateVisible();
}

auto SubtitleRendererItem::isHidden() const -> bool
{
    return d->hidden;
}

auto SubtitleRendererItem::geometryChanged(const QRectF &new_, const QRectF &old) -> void
{
    d->selection.setArea(rect(), devicePixelRatio());
    SimpleTextureItem::geometryChanged(new_, old);
}

auto SubtitleRendererItem::delay() const -> int
{
    return d->delay;
}

auto SubtitleRendererItem::setDelay(int delay) -> void
{
    if (_Change(d->delay, delay))
        rerender();
}

auto SubtitleRendererItem::setPos(double pos) -> void
{
    if (_ChangeF(d->pos, qBound(0.0, pos, 1.0)))
        d->setMargin({{0.0, d->top ? d->pos : 0}, {0.0, d->top ? 0.0 : 1.0 - d->pos}});
}

auto SubtitleRendererItem::setHidden(bool hidden) -> void
{
    if (_Change(d->hidden, hidden))
        d->updateVisible();
}

auto SubtitleRendererItem::setFPS(double fps) -> void
{
    d->selection.setFPS(fps);
}

auto SubtitleRendererItem::fps() const -> double
{
    return d->fps();
}

auto SubtitleRendererItem::deselect(int idx) -> void
{
    if (idx < 0)
        d->selection.clear();
    else if (_InRange(0, idx, d->loaded.size()-1) && d->loaded[idx]->selection())
        d->selection.remove(d->loaded[idx]);
    else
        return;
    if (!d->selecting)
        d->applySelection();
}

auto SubtitleRendererItem::select(int idx) -> void
{
    bool sort = false;
    if (idx < 0) {
        for (auto comp : d->loaded) {
            if (d->selection.prepend(comp))
                sort = true;
        }
    } else if (_InRange(0, idx, d->loaded.size()-1) && !d->loaded[idx]->selection())
        sort = d->selection.prepend(d->loaded[idx]);
    if (sort) {
        d->sort();
        if (!d->selecting)
            d->applySelection();
    }
}

auto SubtitleRendererItem::load(const Subtitle &subtitle, bool select) -> bool
{
    if (subtitle.isEmpty())
        return false;
    const int idx = d->loaded.size();
    for (int i=0; i<subtitle.size(); ++i)
        d->loaded.append(new SubComp(subtitle[i]));
    if (select) {
        d->selecting = true;
        for (int i=d->loaded.size()-1; i>=idx; --i) {
            if (select)
                this->select(i);
        }
        d->selecting = false;
        d->applySelection();
    }
    return true;
}

auto SubtitleRendererItem::start(int time) const -> int
{
    int ret = -1;
    d->selection.forComponents([this, time, &ret] (const SubComp &comp) {
        const auto it = comp.start(time - d->delay, d->fps());
        if (it != comp.end())
            ret = qMax(ret, comp.isBasedOnFrame() ? comp.msec(it.key(), d->fps()) : it.key());
    });
    return ret;
}

auto SubtitleRendererItem::finish(int time) const -> int
{
    int ret = -1;
    d->selection.forComponents([this, time, &ret] (const SubComp &comp) {
        const auto it = comp.finish(time - d->delay, d->fps());
        if (it != comp.end()) {
            const int t = comp.isBasedOnFrame() ? comp.msec(it.key(), d->fps()) : it.key();
            ret = ret == -1 ? t : qMin(ret, t);
        }
    });
    return ret;
}

static bool updateIfEarlier(SubComp::ConstIt it, int &time) {
    if (it->hasWords()) {
        if (time < 0)
            time = it.key();
        else if (it.key() > time)
            time = it.key();
        return true;
    } else
        return false;
}

auto SubtitleRendererItem::current() const -> int
{
    int time = -1;
    d->selection.forImages([this, &time] (const SubCompImage &imageture) {
        if (imageture.isValid())
            updateIfEarlier(imageture.iterator(), time);
    });
    return time;
}

auto SubtitleRendererItem::previous() const -> int
{
    int time = -1;
    d->selection.forImages([this, &time] (const SubCompImage &imageture) {
        if (imageture.isValid()) {
            auto it = imageture.iterator();
            while (it != imageture.component()->begin()) {
                if (updateIfEarlier(--it, time))
                    break;
            }
        }
    });
    return time;
}

auto SubtitleRendererItem::next() const -> int
{
    int time = -1;
    d->selection.forImages([this, &time] (const SubCompImage &imageture) {
        if (imageture.isValid()) {
            auto it = imageture.iterator();
            while (++it != imageture.component()->end()) {
                if (updateIfEarlier(it, time))
                    break;
            }
        }
    });
    return time;
}

auto SubtitleRendererItem::setComponents(const QVector<SubComp> &components) -> void
{
    unload();
    d->loaded.reserve(components.size());
    for (const auto &comp : components)
        d->loaded.push_back(new SubComp(comp));
    for (const auto &comp : d->loaded) {
        if (comp->selection())
            d->selection.prepend(comp);
    }
    d->sort();
    d->applySelection();
}

auto SubtitleRendererItem::models() const -> QVector<SubCompModel*>
{
    return d->selection.models();
}

auto SubtitleRendererItem::customEvent(QEvent *event) -> void
{
    if (event->type() == SubCompSelection::ImagePrepared) {
        if (d->selection.update(_GetData<SubCompImage>(event)))
            d->textChanged = true;
        reserve(UpdateMaterial);
    }
}
