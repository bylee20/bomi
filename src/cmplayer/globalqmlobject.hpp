#ifndef GLOBALQMLOBJECT_HPP
#define GLOBALQMLOBJECT_HPP

#include "stdafx.hpp"
#include <sys/time.h>

enum MemoryUnit {
	ByteUnit = 1, Kilobyte = 1024, Megabyte = 1024*1024, Gigabyte = 1024*1024*1024
};

class SettingsObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString name READ name)
public:
	~SettingsObject() { close(); }
	QString name() const { return m_name; }
	Q_INVOKABLE void open(const QString &name);
	Q_INVOKABLE void close() { if (m_open) {m_set.endGroup(); m_set.endGroup(); m_open = false; m_name.clear();} }
	Q_INVOKABLE void set(const QString &key, const QVariant &var) { if (m_open) m_set.setValue(key, var); }
	Q_INVOKABLE bool getBool(const QString &key, bool def) const { return get(key, def).toBool(); }
	Q_INVOKABLE int getInt(const QString &key, int def) const { return get(key, def).toInt(); }
	Q_INVOKABLE qreal getReal(const QString &key, qreal def) const { return get(key, def).toReal(); }
	Q_INVOKABLE QString getString(const QString &key, const QString &def) const { return get(key, def).toString(); }
	QVariant get(const QString &key, const QVariant &def) const { return m_open ? m_set.value(key, def) : QVariant(); }
private:
	QString m_name; bool m_open = false; QSettings m_set;
};

class UtilObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString monospace READ monospace CONSTANT)
	Q_PROPERTY(double cpu READ cpuUsage)
	Q_PROPERTY(int cores READ cores CONSTANT)
	Q_PROPERTY(double totalMemory READ totalMemory CONSTANT)
	Q_PROPERTY(double memory READ usingMemory)
	Q_PROPERTY(bool cursorVisible READ isCursorVisible NOTIFY cursorVisibleChanged)
	Q_PROPERTY(QString tr READ tr NOTIFY trChanged)
	Q_PROPERTY(bool fullScreen READ isFullScreen NOTIFY fullScreenChanged)
public:
	UtilObject(QObject *parent = nullptr);
	Q_INVOKABLE double textWidth(const QString &text, int size);
	Q_INVOKABLE double textWidth(const QString &text, int size, const QString &family);
	Q_INVOKABLE QString msecToString(int ms) {return Pch::__null_time.addSecs(qRound((double)ms*1e-3)).toString(_L("h:mm:ss"));}
	Q_INVOKABLE void filterDoubleClick() {m_filterDoubleClick = true;}
	Q_INVOKABLE QPointF mousePos(QQuickItem *item);
	Q_INVOKABLE QPointF mapFromSceneTo(QQuickItem *item, const QPointF &scenePos) const;
	Q_INVOKABLE bool execute(const QString &key);
	Q_INVOKABLE double bound(double min, double v, double max) const {return qBound(min, v, max);}
	static void resetDoubleClickFilter() {m_filterDoubleClick = false;}
	static bool isDoubleClickFiltered() {return m_filterDoubleClick;}
	static bool isCursorVisible() {return m_cursor;}
	static void setCursorVisible(bool visible) {if (_Change(m_cursor, visible)) for (auto obj : objs) emit obj->cursorVisibleChanged(m_cursor);}
	static void setMouseReleased(const QPointF &scenePos) {	for (auto obj : objs) emit obj->mouseReleased(scenePos); }
	static double totalMemory(MemoryUnit unit);
	static double usingMemory(MemoryUnit unit);
	static double totalMemory() { return totalMemory(Megabyte); }
	static double usingMemory() { return usingMemory(Megabyte); }
	static double cpuUsage();
	static int cores();
	static quint64 systemTime() { struct timeval t; gettimeofday(&t, 0); return t.tv_sec*1000000u + t.tv_usec; }
	static quint64 processTime(); // usec
	static QString monospace();
	bool isFullScreen() const {return m_fullScreen;}
	static void setFullScreen(bool fs) {if (_Change(m_fullScreen, fs)) for (auto obj : objs) emit obj->fullScreenChanged(m_fullScreen);}
	QString tr() const {return QString();}
	~UtilObject();
signals:
	void trChanged();
	void mouseReleased(const QPointF &scenePos);
	void cursorVisibleChanged(bool cursorVisible);
	void fullScreenChanged(bool fullScreen);
private:
	void changeEvent(QEvent *event) {
		if (event->type() == QEvent::LanguageChange)
			emit trChanged();
	}
	static bool m_fullScreen;
//	static UtilObject &get();
	static bool m_filterDoubleClick, m_pressed, m_cursor;
	static QLinkedList<UtilObject*> objs;

};

#endif // GLOBALQMLOBJECT_HPP
