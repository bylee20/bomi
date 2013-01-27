#include "resourcemonitor.hpp"

#include <sys/sysctl.h>
#include <mach/mach_host.h>
#include <mach/task.h>
#include <unistd.h>
#include <libproc.h>

template<typename T>
static T getSysctl(int name, const T def) {
	T ret; int names[] = {CTL_HW, name}; size_t len = sizeof(def);
	return (sysctl(names, 2u, &ret, &len, NULL, 0) < 0) ? def : ret;
}

double ResourceMonitor::totalMemory(MemoryUnit unit) {
	static const quint64 total = getSysctl(HW_MEMSIZE, (quint64)0);
	return total/(double)unit;
}

int ResourceMonitor::coreCount() {
	static const int count = getSysctl(HW_NCPU, 1);
	return count;
}

double ResourceMonitor::usingMemory(MemoryUnit unit) {
	task_basic_info info; memset(&info, 0, sizeof(info));
	mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &count) != KERN_SUCCESS)
		return 0.0;
	return info.resident_size/(double)unit;
}

quint64 ResourceMonitor::processTime() {
	static const pid_t pid = qApp->applicationPid();
#ifdef Q_OS_MAC
	struct proc_taskinfo info;
	if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &info, sizeof(info)) < 0)
		return 0;
	return info.pti_total_user/1000 + info.pti_total_system/1000;
#endif
#ifdef Q_OS_LINUX
	static const QByteArray path = "/proc" + QByteArray::number(pid) + "/stat";
	uchar buffer[BUFSIZE];
	int pid, ppid, pgrp, session, tty_nr, tpgid; uint flags;
	unsigned long int minflt, cminflt, majflt, cmajflt, utime, stime; char comm[256], state;
	quint64 user, nice, system, idle;
	const auto fd = open(statFilePath.data(), O_RDONLY);
	if (fd < 0)
		return 0;
	int len = ::read(fd, buffer, BUFSIZE);
	if (len > 0) {
		buffer[len] = '\0';
		len = sscanf(buffer.data()
			, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu"
			, &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags
			, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime);
	}
	close(fd);
	return len > 0 ? utime + stime : 0;
#endif
}

double ResourceMonitor::cpuUsage() {
	static quint64 ptime1 = -1, stime1 = -1;
	const quint64 ptime2 = processTime(), stime2 = systemTime();
	const double percent = (double)(ptime2 - ptime1)/(double)(stime2 - stime1)*100.0;
	ptime1 = ptime2; stime1 = stime2;
	return (ptime1 == quint64(-1) || stime1 == quint64(-1)) ? 0.0 : percent;
}

//#ifdef Q_OS_LINUX
//#include <fcntl.h>
//#include <unistd.h>
//struct ProcStat {
//	ProcStat() {
//		const QString path = _L("/proc/") + QString::number(QCoreApplication::applicationPid()) + _L("/stat");
//		statFilePath = path.toLocal8Bit();
//		buffer.resize(BUFSIZ);
//	}
//	QByteArray buffer, statFilePath;
//	int pid, ppid, pgrp, session, tty_nr, tpgid; uint flags;
//	unsigned long int minflt, cminflt, majflt, cmajflt, utime, stime; char comm[256], state;
//	quint64 user, nice, system, idle;
//	bool readProcStat() {
//		const auto fd = open(statFilePath.data(), O_RDONLY);
//		if (fd < 0)
//			return false;
//		int len = ::read(fd, buffer.data(), buffer.size());
//		if (len > 0) {
//			buffer[len] = '\0';
//			len = sscanf(buffer.data()
//				, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu"
//				, &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags
//				, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime);
//		}
//		close(fd);
//		return len > 0;
//	}

//	bool readStat() {
//		const auto fd = open("/proc/stat", O_RDONLY);
//		if (fd < 0)
//			return false;
//		int len = -1;
//		for (;;) {
//			len = ::read(fd, buffer.data(), buffer.size());
//			if (len < 0)
//				break;
//			buffer[len] = '\0';
//			const char *str = strstr(buffer.data(), "cpu");
//			if (!str)
//				continue;
//			str += 3;
//			len = sscanf(str, "%llu %llu %llu %llu", &user, &nice, &system, &idle);
//			break;
//		}
//		close(fd);
//		return len > 0;
//	}
//};
//#endif



ResourceMonitor::ResourceMonitor() {
}
