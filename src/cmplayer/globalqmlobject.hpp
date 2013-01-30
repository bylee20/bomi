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
	Q_PROPERTY(double cpu READ cpuUsage)
	Q_PROPERTY(int cores READ cores CONSTANT)
	Q_PROPERTY(double totalMemory READ totalMemory CONSTANT)
	Q_PROPERTY(double memory READ usingMemory)
public:
	UtilObject(QObject *parent = nullptr);
	Q_INVOKABLE double textWidth(const QString &text, int size);
	Q_INVOKABLE double textWidth(const QString &text, int size, const QString &family);
	Q_INVOKABLE QString msecToString(int ms) {return Pch::__null_time.addSecs(qRound((double)ms*1e-3)).toString(_L("h:mm:ss"));}
	Q_INVOKABLE void filterDoubleClick() {m_filterDoubleClick = true;}
	Q_INVOKABLE QPointF mousePos(QQuickItem *item);
	Q_INVOKABLE QPointF mapFromSceneTo(QQuickItem *item, const QPointF &scenePos) const;
	Q_INVOKABLE bool execute(const QString &key);
	static void resetDoubleClickFilter() {m_filterDoubleClick = false;}
	static bool isDoubleClickFiltered() {return m_filterDoubleClick;}
	static void setMouseReleased(const QPointF &scenePos);
	static double totalMemory(MemoryUnit unit);
	static double usingMemory(MemoryUnit unit);
	static double totalMemory() { return totalMemory(Megabyte); }
	static double usingMemory() { return usingMemory(Megabyte); }
	static double cpuUsage();
	static int cores();
	static quint64 systemTime() { struct timeval t; gettimeofday(&t, 0); return t.tv_sec*1000000u + t.tv_usec; }
	static quint64 processTime(); // usec
	static QString monospace();
	~UtilObject();
signals:
	void mouseReleased(const QPointF &scenePos);
private:
//	static UtilObject &get();
	static bool m_filterDoubleClick, m_pressed;
	static QLinkedList<UtilObject*> objs;

};

#endif // GLOBALQMLOBJECT_HPP
