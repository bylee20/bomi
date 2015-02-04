#include "globalqmlobject.hpp"
#include "player/mainwindow.hpp"
#include "player/rootmenu.hpp"
#include <unistd.h>

UtilObject *UtilObject::object = nullptr;

auto UtilObject::create() -> void
{
    if (!object) { static UtilObject obj; object = &obj; }
}

UtilObject::UtilObject(QObject *parent)
    : QObject(parent) { }

UtilObject::~UtilObject()
{

}

auto UtilObject::setMainWindow(MainWindow *mw) -> void
{
    auto &self = get();
    self.m_main = mw;
    connect(mw, &MainWindow::fullscreenChanged,
            &self, &UtilObject::fullScreenChanged);
}

auto UtilObject::isFullScreen() -> bool
{
    return get().m_main->isFullScreen();
}

auto UtilObject::textWidth(const QString &text, int size,
                           const QString &family) -> double
{
    QFont font(family);
    font.setPixelSize(size);
    QFontMetricsF metrics(font);
    return metrics.width(text);
}

auto UtilObject::textWidth(const QString &text, int size) -> double
{
    return textWidth(text, size, qApp->font().family());
}

auto UtilObject::cpuUsage() -> double
{
    static double percent = 0.0;
    static quint64 ptime1 = processTime(), stime1 = systemTime();
    const quint64 ptime2 = processTime();
    static constexpr quint64 th = 10000;
    if (ptime2 > ptime1 + th) {
        const quint64 stime2 = systemTime();
        if (stime2 > stime1 + th) {
            percent = (double)(ptime2 - ptime1)/(double)(stime2 - stime1)*100.0;
            ptime1 = ptime2; stime1 = stime2;
        }
    }
    return percent;
}

auto UtilObject::mousePos(QQuickItem *item) -> QPointF
{
    if (item && item->window())
        return item->mapFromScene(item->window()->mapFromGlobal(QCursor::pos()));
    return QPointF();
}

auto UtilObject::mapFromSceneTo(QQuickItem *item, const QPointF &scenePos) const -> QPointF
{
    return item ? item->mapFromScene(scenePos) : scenePos;
}


auto UtilObject::execute(const QString &key) -> bool
{
    return RootMenu::execute(key);
}

auto UtilObject::action(const QString &key) -> QObject*
{
    return RootMenu::instance().action(key);
}

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

#ifdef Q_OS_LINUX
#include <fcntl.h>
QString UtilObject::monospace() { return u"monospace"_q; }
static int getField(const char *fileName, const char *fieldName, char *buffer, int size = BUFSIZ) {
    const auto fd = open(fileName, O_RDONLY);
    if (fd < 0)
        return 0;
    int len = ::read(fd, buffer, size);
    int ret = 0;
    if (len > 0) {
        buffer[len] = '\0';
        buffer = strstr(buffer, fieldName);
        if (buffer) {
            buffer += strlen(fieldName);
            do {
                if (!isspace(*buffer) && *buffer != ':')
                    break;
            } while (*(++buffer));
            sscanf(buffer, "%d", &ret);
        }
    }
    close(fd);
    return ret;
}
auto UtilObject::totalMemory(MemoryUnit unit) -> double
{
    static double mem = -1.0;
    if (mem < 0) {
        char buffer[BUFSIZ];
        mem = getField("/proc/meminfo", "MemTotal", buffer)*1000.0;
    }
    return mem/(double)(unit);
}
auto UtilObject::cores() -> int
{
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
    return count;
}
auto UtilObject::usingMemory(MemoryUnit unit) -> double
{
    static char buffer[BUFSIZ];
    return getField("/proc/self/status", "VmRSS", buffer)*1000.0/(double)unit;
}
auto UtilObject::processTime() -> quint64
{
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
}
#endif

auto UtilObject::registerItemToAcceptKey(QQuickItem *item) -> void
{
    if (m_itemsToAcceptKey.contains(item))
        return;
    m_itemsToAcceptKey.insert(item);
    if (item->isVisible())
        m_keyItems.push_front(item);
    connect(item, &QQuickItem::destroyed, [item, this] (QObject *obj) {
        Q_ASSERT(item == obj);
        m_itemsToAcceptKey.remove(item);
        removeKeyItem(item);
    });
    connect(item, &QQuickItem::visibleChanged, [item, this] () {
        removeKeyItem(item);
        if (item->isVisible())
            m_keyItems.push_front(item);
    });
}

auto UtilObject::containsMouse(QQuickItem *item) -> bool
{
    if (!item || !item->window())
        return false;
    return item->contains(mousePos(item));
}
