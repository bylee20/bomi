#ifndef GLOBALQMLOBJECT_HPP
#define GLOBALQMLOBJECT_HPP

#include "stdafx.hpp"
#include <sys/time.h>

enum MemoryUnit {
	Byte = 1, Kilobyte = 1024, Megabyte = 1024*1024, Gigabyte = 1024*1024*1024
};

class UtilObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString monospace READ monospace CONSTANT)
	Q_PROPERTY(bool doubleClicked READ isDoubleClicked WRITE setDoubleClicked)
	Q_PROPERTY(double cpu READ cpuUsage)
	Q_PROPERTY(int cores READ cores)
	Q_PROPERTY(double totalMemory READ totalMemory)
	Q_PROPERTY(double memory READ usingMemory)
public:
	UtilObject(QObject *parent = nullptr);
	Q_INVOKABLE double textWidth(const QString &text, int size) const;
	Q_INVOKABLE double textWidth(const QString &text, int size, const QString &family) const;
	QString monospace() const;
	Q_INVOKABLE QString msecToString(int ms) {return Pch::__null_time.addSecs(qRound((double)ms*1e-3)).toString(_L("h:mm:ss"));}
	static bool isDoubleClicked() {return m_dbc;}
	static void setDoubleClicked(bool dbc) {m_dbc = dbc;}
	static double totalMemory(MemoryUnit unit);
	static double usingMemory(MemoryUnit unit);
	static double totalMemory() { return totalMemory(Megabyte); }
	static double usingMemory() { return usingMemory(Megabyte); }
	static double cpuUsage();
	static int cores();
	static quint64 systemTime() { struct timeval t; gettimeofday(&t, 0); return t.tv_sec*1000000u + t.tv_usec; }
	static quint64 processTime(); // usec
private:
	QFont m_font;
	static bool m_dbc;
};

#endif // GLOBALQMLOBJECT_HPP
