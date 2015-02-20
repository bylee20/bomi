#ifndef SUBTITLERENDERINGTHREAD_HPP
#define SUBTITLERENDERINGTHREAD_HPP

#include "subtitledrawer.hpp"

using SubCompItMap = QMap<int, SubComp::ConstIt>;
using SubCompItMapIt = SubCompItMap::const_iterator;

class SubCompSelection {
public:
    static constexpr int ImagePrepared = QEvent::User+1;
    enum Flag {
        NewDrawer = 1, NewArea = 2, Rebuild = 4, Rerender = 8, Tick = 16
    };
private:
    struct Item;
    class Thread : public QThread {
    public:
        Thread(QMutex *mutex, QWaitCondition *wait, Item *item,
               SubCompSelection *selection, QObject *renderer);
        ~Thread();
        auto setFPS(double fps) -> void;
        auto render(int time, int flags) -> void;
        auto setArea(const QRectF &rect, double dpr) -> void;
        auto setDrawer(const SubtitleDrawer &drawer) -> void;
        auto finish() -> void;
        auto run() -> void;
    private:
        QRectF rect;
        double dpr = 1.0, fps = 1.0;
        SubtitleDrawer drawer;
        int time = 0, flags = 0;
        struct Data; Data *d;
    };
    struct Item {
        auto release() -> void;
        Thread *thread  = nullptr;
        const SubComp *comp = nullptr;
        SubCompImage image{nullptr};
    };
    using List = std::list<Item>;
public:
    SubCompSelection(QObject *renderer);
    ~SubCompSelection();
    auto remove(const SubComp *comp) -> void;
    const SubtitleDrawer &drawer() const;
    auto setDrawer(const SubtitleDrawer &drawer) -> void;
    auto clear() -> void;
    template<class LessThan>
    auto sort(LessThan lt) -> void;
    template<class F>
    auto forComponents(F f) const -> void;
    template<class F>
    auto forImages(F f) const -> void;
    auto setArea(const QRectF &rect, double dpr) -> void;
    auto render(int ms, int flags) -> void;
    auto isEmpty() const -> bool;
    auto prepend(const SubComp *comp) -> bool;
    auto contains(const SubComp *comp) const -> bool;
    auto update(const SubCompImage &pic) -> bool;
    auto fps() const -> double;
    auto setFPS(double fps) -> void;
    auto setMargin(double top, double bottom,
                   double right, double left) -> void;
private:
    auto item(const SubCompImage &image) -> Item*;
    auto find(const SubComp *comp) -> List::iterator;
    auto find(const SubComp *comp) const -> List::const_iterator;
    template<class Func>
    auto forThreads(Func func) -> void;
    List items; mutable QMutex mutex;
    mutable QWaitCondition wait;
    struct Data;
    Data *d;
    QVector<SubCompImage> m_images;
};

inline auto SubCompSelection::Thread::render(int time, int flags) -> void
{ this->time = time; this->flags |= flags; }

inline auto SubCompSelection::Thread::setArea(const QRectF &rect,
                                              double dpr) -> void
{ this->rect = rect; this->dpr = dpr; flags |= NewArea; }

inline auto SubCompSelection::Thread::setDrawer(const SubtitleDrawer &d) -> void
{ this->drawer = d; flags |= NewDrawer; }

template<class LessThan>
inline auto SubCompSelection::sort(LessThan lt) -> void
{
    items.sort([lt] (const Item &lhs, const Item &rhs) {
        return lt(*lhs.comp, *rhs.comp);
    });
}

template<class F>
inline auto SubCompSelection::forComponents(F f) const -> void
{ for (const auto &item : items) f(*item.comp); }

template<class F>
inline auto SubCompSelection::forImages(F f) const -> void
{ for (const auto &item : items) f(item.image); }

inline auto SubCompSelection::render(int ms, int flags) -> void
{ forThreads([this, ms, flags] (Thread *t) { t->render(ms, flags); }); }

inline auto SubCompSelection::Item::release() -> void
{
    _Delete(thread);
    if (comp)
        const_cast<SubComp*>(comp)->selection() = false;
}

inline auto SubCompSelection::contains(const SubComp *comp) const -> bool
{ return find(comp) != items.end(); }

inline auto SubCompSelection::item(const SubCompImage &image) -> Item*
{ return static_cast<Item*>(image.creator()); }

inline auto SubCompSelection::find(const SubComp *comp) -> List::iterator
{
    using std::find_if;
    return std::find_if(items.begin(), items.end(), [comp] (const Item &item) {
        return item.comp == comp;
    });
}

inline auto SubCompSelection::find(const SubComp *comp) const
    -> List::const_iterator
{
    return std::find_if(items.begin(), items.end(), [comp] (const Item &item) {
        return item.comp == comp;
    });
}

template<class Func>
inline auto SubCompSelection::forThreads(Func func) -> void {
    mutex.lock();
    for (const auto &item : items)
        func(item.thread);
    mutex.unlock();
    wait.wakeAll();
}

#endif // SUBTITLERENDERINGTHREAD_HPP
