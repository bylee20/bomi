#include "globalqmlobject.hpp"
#include "rootmenu.hpp"
#include <unistd.h>

void SettingsObject::open(const QString &name) {
	if (m_name != name) {
		close();
		m_set.beginGroup("skin");
		m_set.beginGroup(name);
		m_open = true;
		m_name = name;
	}
}

bool UtilObject::m_fullScreen = false;
bool UtilObject::m_filterDoubleClick = false;
bool UtilObject::m_pressed = false;
bool UtilObject::m_cursor = true;

QLinkedList<UtilObject*> UtilObject::objs;

UtilObject::UtilObject(QObject *parent)
: QObject(parent) {
	objs.append(this);
}

UtilObject::~UtilObject() {
	objs.removeOne(this);
}

double UtilObject::textWidth(const QString &text, int size, const QString &family) {
	QFont font(family);
	font.setPixelSize(size);
	QFontMetricsF metrics(font);
	return metrics.width(text);
}

double UtilObject::textWidth(const QString &text, int size) {
	return textWidth(text, size, qApp->font().family());
}

double UtilObject::cpuUsage() {
	static quint64 ptime1 = -1, stime1 = -1;
	const quint64 ptime2 = processTime(), stime2 = systemTime();
	const double percent = (double)(ptime2 - ptime1)/(double)(stime2 - stime1)*100.0;
	ptime1 = ptime2; stime1 = stime2;
	return (ptime1 == quint64(-1) || stime1 == quint64(-1)) ? 0.0 : percent;
}

QPointF UtilObject::mousePos(QQuickItem *item) {
	if (item && item->window())
		return item->mapFromScene(item->window()->mapFromGlobal(QCursor::pos()));
	return QPointF();
}

QPointF UtilObject::mapFromSceneTo(QQuickItem *item, const QPointF &scenePos) const {
	return item ? item->mapFromScene(scenePos) : scenePos;
}


bool UtilObject::execute(const QString &key) {
	auto action = RootMenu::instance().action(key);
	if (!action)
		return false;
//	if (m_engine) {
		if (action->menu())
			action->menu()->exec(QCursor::pos());
		else
			action->trigger();
//	}
	return true;
}

#ifdef Q_OS_MAC
#include <sys/sysctl.h>
#include <mach/mach_host.h>
#include <mach/task.h>
#include <libproc.h>
QString UtilObject::monospace() { return "monaco"; }
template<typename T>
static T getSysctl(int name, const T def) {
	T ret; int names[] = {CTL_HW, name}; size_t len = sizeof(def);
	return (sysctl(names, 2u, &ret, &len, NULL, 0) < 0) ? def : ret;
}
double UtilObject::totalMemory(MemoryUnit unit) {
	static const quint64 total = getSysctl(HW_MEMSIZE, (quint64)0);
	return total/(double)unit;
}
int UtilObject::cores() { static const int count = getSysctl(HW_NCPU, 1); return count; }
double UtilObject::usingMemory(MemoryUnit unit) {
	task_basic_info info; memset(&info, 0, sizeof(info));
	mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &count) != KERN_SUCCESS)
		return 0.0;
	return info.resident_size/(double)unit;
}
quint64 UtilObject::processTime() {
	static const pid_t pid = qApp->applicationPid();
	struct proc_taskinfo info;
	if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &info, sizeof(info)) < 0)
		return 0;
	return info.pti_total_user/1000 + info.pti_total_system/1000;
}
#endif

#ifdef Q_OS_LINUX
#include <fcntl.h>
QString UtilObject::monospace() { return "monospace"; }
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
double UtilObject::totalMemory(MemoryUnit unit) {
	static double mem = -1.0;
	if (mem < 0) {
		char buffer[BUFSIZ];
		mem = getField("/proc/meminfo", "MemTotal", buffer)*1000.0;
	}
	return mem/(double)(unit);
}
int UtilObject::cores() {
	static int count = -1;
	if (count < 0) {
		QFile file("/proc/cpuinfo");
		count = 0;
		if (!file.open(QFile::ReadOnly | QFile::Text))
			count = 1;
		else {
			char buffer[BUFSIZ];	const char proc[] = "processor";
			while (file.readLine(buffer, BUFSIZ) != -1) {
				buffer[sizeof(proc)-1] = '\0';
				if (!strcmp(buffer, proc))
					++count;
			}
		}
	}
	return count;
}
double UtilObject::usingMemory(MemoryUnit unit) {
	static char buffer[BUFSIZ];
	return getField("/proc/self/status", "VmRSS", buffer)*1000.0/(double)unit;
}
quint64 UtilObject::processTime() {
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
