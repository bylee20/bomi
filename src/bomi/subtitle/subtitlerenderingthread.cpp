#include "subtitlerenderingthread.hpp"
#include "misc/dataevent.hpp"

template<>
inline bool qMapLessThanKey(const SubCompItMapIt &lhs,
                            const SubCompItMapIt &rhs) {
    return lhs.key() < rhs.key();
}

struct SubCompSelection::Thread::Data {
    Item *item = nullptr;
    int time = 0;
    const SubComp *comp = nullptr;
    SubCompItMap its;
    SubCompItMapIt it = its.end();
    QMap<SubCompItMapIt, SubCompImage> pool;
    QObject *receiver = nullptr;
    bool quit = false;
    double fps = 1.0, dpr = 1.0, mul = 1.0;
    QMutex *mutex; QWaitCondition *wait;
    QRectF rect; SubtitleDrawer drawer;
    SubCompSelection *selection = nullptr;

    SubComp::ConstIt iterator(int time) const { return comp->start(time, fps); }
    auto newPicture(SubCompItMapIt it)
    {
        auto pic = pool.insert(it, SubCompImage(comp, *it, item));
        drawer.draw(*pic, rect, dpr);
        return pic;
    }
    auto update()
    {
        if (quit)
            return;
        auto post = [this] (const SubCompImage &pic)
            { _PostEvent(receiver, ImagePrepared, pic); };
        if (it != its.end()) {
            auto cache = pool.find(it);
            if (cache == pool.end())
                cache = newPicture(it);
            post(*cache);
        } else
            post(comp);
    }

    auto fillCache()
    {
        if (it == its.end())
            return;
        auto iit = pool.begin();
        while (iit != pool.end() && iit.key().key() < it.key())
            iit = pool.erase(iit);
        Q_ASSERT(iit.key() == it);
        auto key = it;
        const int size = qMin(2, pool.size()+1);
        for (int i=0; i<size; ++i) {
            ++iit; ++key;
            if (key == its.end())
                break;
            if (iit == pool.end())
                iit = newPicture(key);
        }
    }

    auto draw(bool force)
    {
        auto iit = --its.upperBound(time);
        if (force || it != iit) {
            if (it == its.end() || ++it != iit) {
                pool.clear();
                it = iit;
                update();
            } else {
                update();
                fillCache();
            }
        }
    }

    auto rebuild()
    {
        pool.clear();
        its.clear();
        it = its.end();
        for (auto iit = comp->begin(); iit != comp->end(); ++iit)
            its.insert(comp->toTime(iit.key(), fps), iit);
    }
};

SubCompSelection::Thread::Thread(QMutex *mutex, QWaitCondition *wait,
                                 Item *item, SubCompSelection *selection,
                                 QObject *renderer)
    : QThread()
    , d(new Data)
{
    d->item = item;
    d->comp = item->comp;
    d->receiver = renderer;
    d->mutex = mutex;
    d->wait = wait;
    d->selection = selection;
}

SubCompSelection::Thread::~Thread()
{
    finish();
    delete d;
}

auto SubCompSelection::Thread::setFPS(double fps) -> void
{
    this->fps = fps;
    flags |= Rebuild;
}

auto SubCompSelection::Thread::finish() -> void
{
    d->quit = true;
    d->wait->wakeAll();
    if (!wait(5000))
        terminate();
}

auto SubCompSelection::Thread::run() -> void
{
    static constexpr int NewOption = NewDrawer | NewArea;
    static constexpr int ForceUpdate = Rerender | Rebuild | NewOption;
    int flags = 0;
    while (!d->quit) {
        QMutexLocker locker(d->mutex);
        if (!(this->flags & ForceUpdate))
            d->wait->wait(d->mutex);
        if (d->quit)
            break;
        flags = this->flags;
        this->flags = 0;
        d->time = time;
        d->fps = fps;
        if (flags & NewOption) {
            if (flags & NewDrawer)
                d->drawer = drawer;
            if (flags & NewArea) {
                d->rect = rect;
                d->dpr = dpr;
            }
        }
        locker.unlock();
        if (d->quit)
            break;
        if (flags & Rebuild)
            d->rebuild();
        if (flags & NewOption)
            d->pool.clear();
        if (d->quit)
            break;
        if (d->time > 0 && d->fps > 0.0 && !d->its.isEmpty())
            d->draw(flags & ForceUpdate);
    }
}

/******************************************************************************/

struct SubCompSelection::Data {
    QMutex mutex;
    QWaitCondition wait;
    QObject *renderer = nullptr;
    SubtitleDrawer drawer;
    QRectF rect;
    double dpr = 1.0, fps = 30.0;
};

SubCompSelection::SubCompSelection(QObject *renderer)
    : d(new Data)
{
    d->renderer = renderer;
}

SubCompSelection::~SubCompSelection()
{
    delete d;
}

auto SubCompSelection::remove(const SubComp *comp) -> void
{
    auto it = find(comp);
    if (it != items.end()) {
        it->release();
        items.erase(it);
    }
}

auto SubCompSelection::drawer() const -> const SubtitleDrawer&
{
    return d->drawer;
}

auto SubCompSelection::setDrawer(const SubtitleDrawer &drawer) -> void
{
    d->drawer = drawer;
    forThreads([this] (Thread *t) { t->setDrawer(d->drawer); });
}

auto SubCompSelection::clear() -> void
{
    for (auto &item : items)
        item.thread->finish();
    wait.wakeAll();
    qApp->removePostedEvents(d->renderer, ImagePrepared);
    for (auto &item : items)
        item.release();
    items.clear();
}

auto SubCompSelection::setArea(const QRectF &rect, double dpr) -> void
{
    if (d->rect == rect && d->dpr == dpr)
        return;
    d->rect = rect; d->dpr = dpr;
    forThreads([this] (Thread *thread) { thread->setArea(d->rect, d->dpr); });
}

auto SubCompSelection::isEmpty() const -> bool
{
    for (const auto &item : items)
        if (item.comp->hasWords())
            return false;
    return true;
}

auto SubCompSelection::prepend(const SubComp *comp) -> bool
{
    if (contains(comp) || !comp)
        return false;
    const_cast<SubComp*>(comp)->selection() = true;
    items.push_front(Item());
    auto &item = items.front();
    item.comp = comp;
    item.thread = new Thread(&mutex, &wait, &item, this, d->renderer);
    item.thread->setFPS(d->fps);
    item.thread->setDrawer(d->drawer);
    item.thread->setArea(d->rect, d->dpr);
    item.thread->start();
    return true;
}

auto SubCompSelection::fps() const -> double
{
    return d->fps;
}

auto SubCompSelection::setFPS(double fps) -> void
{
    if (_Change(d->fps, fps))
        forThreads([this, fps] (Thread *t) { t->setFPS(fps); });
}

auto SubCompSelection::update(const SubCompImage &image) -> bool
{
    auto item = this->item(image);
    if (!item)
        return false;
    item->image = image;
    return true;
}

auto SubCompSelection::setMargin(double top, double bottom,
                                 double right, double left) -> void
{
    Margin margin;
    margin.top = top;     margin.bottom = bottom;
    margin.right = right; margin.left = left;
    d->drawer.setMargin(margin);
}
