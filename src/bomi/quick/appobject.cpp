#include "appobject.hpp"
#include "tmp/algorithm.hpp"
#include "windowobject.hpp"
#include "player/rootmenu.hpp"
#include <unistd.h>
#include <fcntl.h>

static int getField(const char *file, const char *field) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    static char buffer[BUFSIZ]; // should be called in GUI thread!!
    const auto fd = open(file, O_RDONLY);
    if (fd < 0)
        return 0;
    int len = ::read(fd, buffer, BUFSIZ);
    int ret = 0;
    if (len > 0) {
        buffer[len] = '\0';
        auto pos = strstr(buffer, field);
        if (pos) {
            pos += strlen(field);
            do {
                if (!isspace(*pos) && *pos != ':')
                    break;
            } while (*(++pos));
            sscanf(pos, "%d", &ret);
        }
    }
    close(fd);
    return ret;
}

auto reg_app_object() -> void {
    qmlRegisterSingletonType<AppObject>("bomi", 1, 0, "App",
                                        _QmlSingleton<AppObject>);
    qmlRegisterType<WindowObject>();
    qmlRegisterType<MemoryObject>();
    qmlRegisterType<CpuObject>();
    qmlRegisterType<MouseObject>();
}

MemoryObject::MemoryObject()
{
    static double total = -1.0;
    if (total < 0)
        total = getField("/proc/meminfo", "MemTotal")*1000.0;
    m_total = total/(1024*1024);

    auto setUsage = [=] () {
        auto mem = getField("/proc/self/status", "VmRSS")*1000.0/(1024*1024);
        if (_Change(m_usage, mem))
            emit usageChanged();
    };
    setUsage();

    connect(&m_timer, &QTimer::timeout, this, setUsage);
    m_timer.setInterval(500);
    m_timer.start();
}

MemoryObject::~MemoryObject()
{
    m_timer.stop();
}

/******************************************************************************/

#ifdef Q_OS_MAC
#include <sys/sysctl.h>
#include <mach/mach_host.h>
#include <mach/task.h>
#include <libproc.h>
QString UtilObject::monospace() { return u"monaco"_q; }
template<class T>
static T getSysctl(int name, const T def) {
    T ret; int names[] = {CTL_HW, name}; size_t len = sizeof(def);
    return (sysctl(names, 2u, &ret, &len, NULL, 0) < 0) ? def : ret;
}
auto UtilObject::totalMemory(MemoryUnit unit) -> double
{
    static const quint64 total = getSysctl(HW_MEMSIZE, (quint64)0);
    return total/(double)unit;
}
int UtilObject::cores() { static const int count = getSysctl(HW_NCPU, 1); return count; }
auto UtilObject::usingMemory(MemoryUnit unit) -> double
{
    task_basic_info info; memset(&info, 0, sizeof(info));
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &count) != KERN_SUCCESS)
        return 0.0;
    return info.resident_size/(double)unit;
}
auto UtilObject::processTime() -> quint64
{
    static const pid_t pid = qApp->applicationPid();
    struct proc_taskinfo info;
    if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &info, sizeof(info)) < 0)
        return 0;
    return info.pti_total_user/1000 + info.pti_total_system/1000;
}
#endif

CpuObject::CpuObject()
{
    auto processTime = [] () -> quint64 {
        static char buffer[BUFSIZ];
        static const quint64 tick = sysconf(_SC_CLK_TCK);
        int pid, ppid, pgrp, session, tty_nr, tpgid; uint flags;
        unsigned long int minflt, cminflt, majflt, cmajflt, utime, stime; char comm[256], state;
        const auto fd = open("/proc/self/stat", O_RDONLY);
        if (fd < 0)
            return 0;
        int len = ::read(fd, buffer, BUFSIZ);
        if (len > 0) {
            buffer[len] = '\0';
            len = sscanf(buffer
                , "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu"
                , &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags
                , &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime);
        }
        close(fd);
        return len > 0 ? quint64(utime + stime)*Q_UINT64_C(1000000)/tick : 0;
    };

    auto setUsage = [=] () {
        double usage = 0.0;
        if (!m_pt) {
            m_pt = processTime();
            m_st = _SystemTime();
            return;
        }
        const auto pt = processTime();
        const auto st = _SystemTime();
        static constexpr quint64 th = 10000;
        if (pt > m_pt + th && st > m_st + th) {
            usage = (pt - m_pt)/(double)(st - m_st)*100.0;
            m_pt = pt;
            m_st = st;
        }
        if (_Change(m_usage, usage))
            emit usageChanged();
    };
    setUsage();

    static int count = -1;
    if (count < 0) {
        QFile file(u"/proc/cpuinfo"_q);
        count = 0;
        if (!file.open(QFile::ReadOnly | QFile::Text))
            count = 1;
        else {
            char buffer[BUFSIZ];
            const char proc[] = "processor";
            while (file.readLine(buffer, BUFSIZ) != -1) {
                buffer[sizeof(proc)-1] = '\0';
                if (!strcmp(buffer, proc))
                    ++count;
            }
        }
    }
    m_cores = count;

    connect(&m_timer, &QTimer::timeout, this, setUsage);
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
