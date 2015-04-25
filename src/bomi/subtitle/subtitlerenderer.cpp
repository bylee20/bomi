#include "subtitlerenderer.hpp"
#include "subtitlerenderingthread.hpp"
#include "misc/dataevent.hpp"
#include "enum/autoselectmode.hpp"
#include "opengl/opengltexture2d.hpp"
#include "opengl/opengltexturebinder.hpp"

struct SubtitleShaderData : public SubtitleRenderer::ShaderData {
    const OpenGLTexture2D *texture, *bbox;
    QColor bboxColor;
};

struct SubtitleShader : public SubtitleRenderer::ShaderIface {
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
    void update(QOpenGLShaderProgram *prog,
                SubtitleRenderer::ShaderData *data) override {
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

struct SubtitleRenderer::Data {
    Data(SubtitleRenderer *p): p(p) {}
    SubtitleRenderer *p = nullptr;
    QList<SubComp*> loaded;
    QSize imageSize{0, 0};
    SubtitleDrawer drawer;
    int delay = 0, msec = 0, lastTime = -1;
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

    auto find(int id) const -> SubComp*
    {
        for (auto comp : loaded) {
            if (comp->id() == id)
                return comp;
        }
        return nullptr;
    }

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
        emit p->selectionChanged();
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

SubtitleRenderer::SubtitleRenderer(QQuickItem *parent)
    : SimpleTextureItem(parent)
    , d(new Data(this))
{
    d->drawer.setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    d->updateDrawer();
}

SubtitleRenderer::~SubtitleRenderer() {
    unload();
    delete d;
}

auto SubtitleRenderer::isTopAligned() const -> bool
{
    return d->top;
}

auto SubtitleRenderer::render(int ms) -> void
{
    d->msec = ms;
    d->render(SubCompSelection::Tick);
}

auto SubtitleRenderer::rerender() -> void
{
    d->render(SubCompSelection::Rerender);
}

auto SubtitleRenderer::createShader() const -> ShaderIface*
{
    return new SubtitleShader;
}

auto SubtitleRenderer::initializeGL() -> void
{
    SimpleTextureItem::initializeGL();
    texture().create();
    d->bbox.create();
}

auto SubtitleRenderer::finalizeGL() -> void
{
    SimpleTextureItem::finalizeGL();
    d->bbox.destroy();
    texture().destroy();
}

auto SubtitleRenderer::text() const -> const RichTextDocument&
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

auto SubtitleRenderer::setPriority(const QStringList &priority) -> void
{
    for (int i=0; i< priority.size(); ++i)
        d->langMap[priority[i]] = priority.size()-i;
}

auto SubtitleRenderer::setStyle(const OsdStyle &style) -> void
{
    d->drawer.setStyle(style);
    d->updateDrawer();
}

auto SubtitleRenderer::draw(const QRectF &rect, QRectF *put) const -> QImage
{
    if (d->hidden)
        return QImage();
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

auto SubtitleRenderer::pos() const -> double
{
    return d->pos;
}

auto SubtitleRenderer::componentsCount() const -> int
{
    return d->loaded.count();
}

auto SubtitleRenderer::components() const -> QVector<const SubComp *>
{
    QVector<const SubComp*> list;
    list.reserve(d->loaded.size());
    for (auto loaded : d->loaded)
        list.push_back(loaded);
    return list;
}

auto SubtitleRenderer::setTopAligned(bool top) -> void
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

auto SubtitleRenderer::style() const -> const OsdStyle&
{
    return d->drawer.style();
}

auto SubtitleRenderer::unload() -> void
{
    d->selection.clear();
    qDeleteAll(d->loaded);
    d->loaded.clear();
    setVisible(false);
    d->empty = true;
    d->textChanged = true;
    emit selectionChanged();
}

auto SubtitleRenderer::updateVertex(Vertex *vertex) -> void
{
    const auto dpr = devicePixelRatio();
    const QRectF r(d->drawer. pos(d->imageSize/dpr, rect()), d->imageSize/dpr);
    OGL::CoordAttr::fillTriangleStrip(vertex, &Vertex::position,
                                      r.topLeft(), r.bottomRight());
}

auto SubtitleRenderer::createData() const -> ShaderData*
{
    auto data = new SubtitleShaderData;
    data->texture = &texture();
    data->bbox = &d->bbox;
    return data;
}

auto SubtitleRenderer::updateData(ShaderData *sd) -> void
{
    auto data = static_cast<SubtitleShaderData*>(sd);
    updateTexture(&texture());
    data->bboxColor = d->drawer.style().bbox.color;
}

auto SubtitleRenderer::updateTexture(OpenGLTexture2D *texture) -> void
{
    d->imageSize = {0, 0};
    const int spacing = d->drawer.style().font.height()
            * d->drawer.scale(geometry())
            * d->drawer.style().spacing.paragraph + 0.5;
    int lastTime = -1;
    d->selection.forImages([&] (const SubCompImage &image) {
        if (d->imageSize.width() < image.width())
            d->imageSize.rwidth() = image.width();
        d->imageSize.rheight() += image.height() + spacing;
        if (image.isValid())
            lastTime = std::max(image.iterator().key(), lastTime);
    });
    d->imageSize.rheight() -= spacing;
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
            y += image.height() + spacing;
        });
        reserve(UpdateGeometry, false);
    }
    if (_Change(d->lastTime, lastTime))
        emit updated(d->lastTime);
}

auto SubtitleRenderer::afterUpdate() -> void
{
    d->updateVisible();
}

auto SubtitleRenderer::isHidden() const -> bool
{
    return d->hidden;
}

auto SubtitleRenderer::geometryChanged(const QRectF &new_, const QRectF &old) -> void
{
    d->selection.setArea(rect(), devicePixelRatio());
    SimpleTextureItem::geometryChanged(new_, old);
}

auto SubtitleRenderer::delay() const -> int
{
    return d->delay;
}

auto SubtitleRenderer::setDelay(int delay) -> void
{
    if (_Change(d->delay, delay))
        rerender();
}

template<class T, class F = std::equal_to<T>>
auto _Change2(T &the, const T &one, F equal = std::equal_to<T>()) -> bool
{ if (!equal(the, one)) {the = one; return true;} return false; }

template<class U, class T>
auto _Change2(T &the, const T &one, bool(*equal)(U,U)) -> bool
{
    return _Change2(the, one, [equal] (const T &t1, const T &t2) { return equal(t1, t2); });
}

auto SubtitleRenderer::setPos(double pos) -> void
{
    if (_Change2<double>(d->pos, qBound(0.0, pos, 1.0), qFuzzyCompare))
        d->setMargin({{0.0, d->top ? d->pos : 0}, {0.0, d->top ? 0.0 : 1.0 - d->pos}});
}

auto SubtitleRenderer::setHidden(bool hidden) -> void
{
    if (_Change(d->hidden, hidden))
        d->updateVisible();
}

auto SubtitleRenderer::setFPS(double fps) -> void
{
    d->selection.setFPS(fps);
}

auto SubtitleRenderer::fps() const -> double
{
    return d->fps();
}

auto SubtitleRenderer::deselect(int id) -> void
{
    if (id < 0) // deselect all
        d->selection.clear();
    else {
        auto comp = d->find(id);
        if (!comp || !comp->selection())
            return;
        d->selection.remove(comp);
    }
    if (!d->selecting)
        d->applySelection();
}

auto SubtitleRenderer::select(int id) -> void
{
    bool sort = false;
    if (id < 0) { // select all
        for (auto comp : d->loaded) {
            if (d->selection.prepend(comp))
                sort = true;
        }
    } else {
        auto comp = d->find(id);
        if (comp && !comp->selection())
            sort = d->selection.prepend(comp);
    }
    if (sort) {
        d->sort();
        if (!d->selecting)
            d->applySelection();
    }
}

//auto SubtitleRendererItem::load(const Subtitle &subtitle, bool select) -> bool
//{
//    if (subtitle.isEmpty())
//        return false;
//    const int idx = d->loaded.size();
//    for (int i=0; i<subtitle.size(); ++i)
//        d->loaded.append(new SubComp(subtitle[i]));
//    if (select) {
//        d->selecting = true;
//        for (int i=d->loaded.size()-1; i>=idx; --i) {
//            if (select)
//                this->select(i);
//        }
//        d->selecting = false;
//        d->applySelection();
//    }
//    return true;
//}

auto SubtitleRenderer::lastUpdatedTime() const -> int
{
    return d->lastTime;
}

auto SubtitleRenderer::start(int time) const -> int
{
    int ret = -1;
    d->selection.forComponents([this, time, &ret] (const SubComp &comp) {
        const auto it = comp.start(time - d->delay, d->fps());
        if (it != comp.end())
            ret = qMax(ret, comp.isBasedOnFrame() ? comp.msec(it.key(), d->fps()) : it.key());
    });
    return ret;
}

auto SubtitleRenderer::finish(int time) const -> int
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

auto SubtitleRenderer::current() const -> int
{
    int time = -1;
    d->selection.forImages([this, &time] (const SubCompImage &imageture) {
        if (imageture.isValid())
            updateIfEarlier(imageture.iterator(), time);
    });
    return time;
}

auto SubtitleRenderer::previous() const -> int
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

auto SubtitleRenderer::next() const -> int
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

auto SubtitleRenderer::addComponents(const QVector<SubComp> &components) -> void
{
    if (components.isEmpty())
        return;
    const int idx = d->loaded.size();
    for (auto &comp : components)
        d->loaded.append(new SubComp(comp));
    d->selecting = true;
    bool sort = false;
    for (int i=d->loaded.size()-1; i>=idx; --i) {
        if (d->loaded[i]->selection())
            sort = d->selection.prepend(d->loaded[i]);
    }
    d->selecting = false;
    if (sort) {
        d->sort();
        d->applySelection();
    }
}

auto SubtitleRenderer::setComponents(const QVector<SubComp> &components) -> void
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

auto SubtitleRenderer::selection() const -> QVector<SubComp>
{
    QVector<SubComp> selection;
    d->selection.forComponents([&] (const SubComp &comp)
                               { selection.push_back(comp); });
    return selection;
}

auto SubtitleRenderer::customEvent(QEvent *event) -> void
{
    if (event->type() == SubCompSelection::ImagePrepared) {
        if (d->selection.update(_GetData<SubCompImage>(event)))
            d->textChanged = true;
        reserve(UpdateMaterial);
    }
}

auto SubtitleRenderer::toTrackList() const -> StreamList
{
    StreamList list(StreamInclusiveSubtitle);
    for (int i = 0; i < d->loaded.size(); ++i)
        list.insert(d->loaded[i]->toTrack());
    return list;
}
