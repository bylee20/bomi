#include "appobject.hpp"
#include "formatobject.hpp"
#include "tmp/algorithm.hpp"
#include "windowobject.hpp"
#include "player/rootmenu.hpp"
#include "os/os.hpp"
#include <QQmlEngine>

extern "C" {
int av_cpu_count(void);
}

MemoryObject::MemoryObject()
{
    m_total = OS::totalMemory();
    m_usage = OS::usingMemory();

    connect(&m_timer, &QTimer::timeout, this, [=] () {
        if (_Change(m_usage, OS::usingMemory()))
            emit usageChanged();
    });
    m_timer.setInterval(500);
    m_timer.start();
}

MemoryObject::~MemoryObject()
{
    m_timer.stop();
}

/******************************************************************************/


CpuObject::CpuObject()
{
    m_pt = OS::processTime();
    m_st = OS::systemTime();
    m_cores = av_cpu_count();

    connect(&m_timer, &QTimer::timeout, this, [=] () {
        double usage = 0.0;
        const auto pt = OS::processTime();
        const auto st = OS::systemTime();
        static constexpr quint64 th = 10000;
        if (pt > m_pt + th && st > m_st + th) {
            usage = (pt - m_pt)/(double)(st - m_st)*100.0;
            _R(m_pt, m_st) = _T(pt, st);
        }
        if (_Change(m_usage, usage))
            emit usageChanged();
    });
    m_timer.setInterval(500);
    m_timer.start();
}

CpuObject::~CpuObject()
{
    m_timer.stop();
}

/******************************************************************************/

AppObject::StaticData AppObject::s;

auto AppObject::setWindow(MainWindow *window) -> void
{
    static WindowObject o;
    o.set(window);
    s.window = &o;
}

auto AppObject::description(const QString &actionId) const -> QString
{
    return RootMenu::instance().description(actionId);
}

auto AppObject::textWidth(const QString &text, int size, const QString &family) const -> double
{
    QFont font(family);
    font.setPixelSize(size);
    QFontMetricsF metrics(font);
    return metrics.width(text);
}

auto AppObject::textWidth(const QString &text, int size) const -> double
{
    return textWidth(text, size, qApp->font().family());
}

auto AppObject::execute(const QString &key) const -> bool
{
    return RootMenu::execute(key);
}

auto AppObject::action(const QString &key) const -> QObject*
{
    return RootMenu::instance().action(key);
}

auto AppObject::registerToAccept(QQuickItem *item, Events e) -> void
{
    if (s.itemsToAccept.contains(item))
        return;
    s.itemsToAccept.insert(item, e);
    if (item->isVisible())
        s.orderToAccept.push_front(item);
    auto pop = [] (QQuickItem *item) -> bool
    {
        auto it = tmp::find(s.orderToAccept, item);
        if (it == s.orderToAccept.end())
            return false;
        s.orderToAccept.erase(it);
        return true;
    };
    connect(item, &QQuickItem::destroyed, [=] (QObject *obj) {
        Q_ASSERT(item == obj);
        s.itemsToAccept.remove(item);
        pop(item);
    });
    connect(item, &QQuickItem::visibleChanged, [=] () {
        pop(item);
        if (item->isVisible())
            s.orderToAccept.push_front(item);
    });
}

auto AppObject::itemToAccept(Event event, const QPointF &scenePos) -> QQuickItem*
{
    if (event & MouseEvent) {
        for (auto item : s.orderToAccept) {
            if (!(s.itemsToAccept[item] & event))
                continue;
            if (item->contains(item->mapFromScene(scenePos)))
                return item;
        }
    } else {
        for (auto item : s.orderToAccept) {
            if (s.itemsToAccept[item] & event)
                return item;
        }
    }
    return nullptr;
}

auto AppObject::delete_(QObject *o) -> void
{
    QQmlEngine::setObjectOwnership(o, QQmlEngine::CppOwnership);
    delete o;
}
