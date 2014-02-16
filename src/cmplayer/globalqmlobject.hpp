#ifndef GLOBALQMLOBJECT_HPP
#define GLOBALQMLOBJECT_HPP

#include "stdafx.hpp"

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
	~UtilObject();
	Q_INVOKABLE double textWidth(const QString &text, int size);
	Q_INVOKABLE double textWidth(const QString &text, int size, const QString &family);
	Q_INVOKABLE QString msecToString(int ms) {return Pch::_NullTime.addSecs(qRound((double)ms*1e-3)).toString(_L("h:mm:ss"));}
	Q_INVOKABLE QString secToString(int s) {return Pch::_NullTime.addSecs(s).toString(_L("h:mm:ss"));}
	Q_INVOKABLE QPointF mousePos(QQuickItem *item);
	Q_INVOKABLE QPointF mapFromSceneTo(QQuickItem *item, const QPointF &scenePos) const;
	Q_INVOKABLE bool execute(const QString &key);
	Q_INVOKABLE static QObject *action(const QString &key);
	Q_INVOKABLE double bound(double min, double v, double max) const {return qBound(min, v, max);}
	Q_INVOKABLE void showToolTip(QQuickItem *item, const QPointF &pos, const QString &text) { QToolTip::showText(item->window()->mapToGlobal(item->mapToScene(pos).toPoint()), text); }
	Q_INVOKABLE void hideToolTip() { QToolTip::hideText(); }
	Q_INVOKABLE void filterDoubleClick() { m_filterDoubleClick = true; }
	static bool isDoubleClickFiltered() { return m_filterDoubleClick; }
	static void resetFilterDoubleClick() { m_filterDoubleClick = false; }
	static bool isCursorVisible() {return m_cursor;}
	static void setCursorVisible(bool visible) {if (_Change(m_cursor, visible)) for (auto obj : objs) emit obj->cursorVisibleChanged(m_cursor);}
	Q_INVOKABLE void setCursor(QQuickItem *item, Qt::CursorShape cursor) { if (item) item->setCursor(cursor); }
	Q_INVOKABLE void unsetCursor(QQuickItem *item) { if (item) item->unsetCursor(); }
	static double totalMemory(MemoryUnit unit);
	static double usingMemory(MemoryUnit unit);
	static double totalMemory() { return totalMemory(Megabyte); }
	static double usingMemory() { return usingMemory(Megabyte); }
	static double cpuUsage();
	static int cores();
	static quint64 systemTime() { return _SystemTime(); }
	static quint64 processTime(); // usec
	static QString monospace();
	bool isFullScreen() const {return m_fullScreen;}
	static void setFullScreen(bool fs) {if (_Change(m_fullScreen, fs)) for (auto obj : objs) emit obj->fullScreenChanged(m_fullScreen);}
	QString tr() const {return QString();}
	Q_INVOKABLE void registerItemToAcceptKey(QQuickItem *item);
	static QQuickItem *itemToAcceptKey() { return d->keyItems.isEmpty() ? nullptr : d->keyItems.front(); }
	static void setItemPressed(QQuickItem *item);
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
	static bool m_fullScreen, m_cursor, m_filterDoubleClick;
//	static UtilObject &get();
	static QLinkedList<UtilObject*> objs;
	struct Data {
		QSet<QQuickItem*> itemsToAcceptKey;
		QLinkedList<QQuickItem*> keyItems;
		bool removeKeyItem(QQuickItem *item) {
			auto it = _Find(keyItems.begin(), keyItems.end(), item);
			if (it == keyItems.end())
				return false;
			keyItems.erase(it);
			return true;
		}
	};

	static Data data;
	static Data *d;
};

#endif // GLOBALQMLOBJECT_HPP
