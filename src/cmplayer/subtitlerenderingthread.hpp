#ifndef SUBTITLERENDERINGTHREAD_HPP
#define SUBTITLERENDERINGTHREAD_HPP

#include "stdafx.hpp"
#include "subtitle.hpp"
#include "subtitledrawer.hpp"
#include "subtitlemodel.hpp"

using SubCompItMap = QMap<int, SubComp::ConstIt>;
using SubCompItMapIt = SubCompItMap::const_iterator;

class SubCompSelection {
public:
    static constexpr int ImagePrepared = QEvent::User+1;
    enum Flag {NewDrawer = 1, NewArea = 2, Rebuild = 4, Rerender = 8, Tick = 16};
private:
    struct Item;
    class Thread : public QThread {
    public:
        Thread(QMutex *mutex, QWaitCondition *wait, Item *item, SubCompSelection *selection, QObject *renderer);
        ~Thread();
        void setFPS(double fps);
        void render(int time, int flags) { this->time = time; this->flags |= flags; }
        void setArea(const QRectF &rect, double dpr) { this->rect = rect; this->dpr = dpr; flags |= NewArea; }
        void setDrawer(const SubtitleDrawer &drawer) { this->drawer = drawer; flags |= NewDrawer; }
        void finish();
        void run();
    private:
        QRectF rect; double dpr = 1.0; SubtitleDrawer drawer;
        int time = 0, flags = 0; double fps = 1.0;
        struct Data; Data *d;
    };
    struct Item {
        void release() {
            delete thread; delete model; if (comp) const_cast<SubComp*>(comp)->selection() = false;
        }
        Thread *thread  = nullptr;
        const SubComp *comp = nullptr;
        SubCompImage image = {nullptr};
        SubCompModel *model = nullptr;
    };
    using List = std::list<Item>;
public:
    SubCompSelection(QObject *renderer);
    ~SubCompSelection();
    QVector<SubCompModel*> models() const;
    void remove(const SubComp *comp);
    const SubtitleDrawer &drawer() const;
    void setDrawer(const SubtitleDrawer &drawer);
    void clear();
    template<typename LessThan>
    void sort(LessThan lt) {
        items.sort([lt] (const Item &lhs, const Item &rhs) { return lt(*lhs.comp, *rhs.comp); });
    }
    void setArea(const QRectF &rect, double dpr);
    void render(int ms, int flags) { forThreads([this, ms, flags] (Thread *t) { t->render(ms, flags); }); }
    bool isEmpty() const;
    bool prepend(const SubComp *comp);
    bool contains(const SubComp *comp) const { return find(comp) != items.end(); }
    template<typename F> void forComponents(F f) const { for (const auto &item : items) f(*item.comp); }
    template<typename F> void forImages    (F f) const { for (const auto &item : items) f(item.image); }
    bool update(const SubCompImage &pic);
    double fps() const;
    void setFPS(double fps);
    void setMargin(double top, double bottom, double right, double left);
private:
    Item *item(const SubCompImage &image) { return static_cast<Item*>(image.creator()); }
    List::iterator find(const SubComp *comp) {
        return std::find_if(items.begin(), items.end()
            , [comp] (const Item &item) { return item.comp == comp; });
    }
    List::const_iterator find(const SubComp *comp) const {
        return std::find_if(items.begin(), items.end()
            , [comp] (const Item &item) { return item.comp == comp; });
    }
    template<typename Func>
    void forThreads(Func func) {
        mutex.lock();
        for (const auto &item : items)
            func(item.thread);
        mutex.unlock();
        wait.wakeAll();
    }
    List items; mutable QMutex mutex; mutable QWaitCondition wait;
    struct Data; Data *d;
    QVector<SubCompImage> m_images;
};

#endif // SUBTITLERENDERINGTHREAD_HPP
