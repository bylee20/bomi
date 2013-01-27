#ifndef RESOURCEMONITOR_HPP
#define RESOURCEMONITOR_HPP

#include "stdafx.hpp"
#include <sys/time.h>

enum MemoryUnit {
	Byte = 1, Kilobyte = 1024, Megabyte = 1024*1024, Gigabyte = 1024*1024*1024
};

class ResourceMonitor {
public:
	static double totalMemory(MemoryUnit unit = Megabyte);
	static double usingMemory(MemoryUnit unit = Megabyte);
	static double cpuUsage();
	static int coreCount();
	static inline quint64 systemTime() { struct timeval t; gettimeofday(&t, 0); return t.tv_sec*1000000u + t.tv_usec; }
	static quint64 processTime(); // usec
private:
	ResourceMonitor();
};

#endif // RESOURCEMONITOR_HPP
