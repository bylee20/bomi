#ifndef GLOBALQMLOBJECT_HPP
#define GLOBALQMLOBJECT_HPP

#include "stdafx.hpp"

enum MemoryUnit {
    ByteUnit = 1, Kilobyte = 1024, Megabyte = 1024*1024, Gigabyte = 1024*1024*1024
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
    ~UtilObject();
    Q_INVOKABLE static double textWidth(const QString &text, int size);
    Q_INVOKABLE static double textWidth(const QString &text, int size, const QString &family);
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
    static bool isDoubleClickFiltered() { return get().m_filterDoubleClick; }
    static void resetFilterDoubleClick() { get().m_filterDoubleClick = false; }
    static bool isCursorVisible() {return get().m_cursor;}
    static void setCursorVisible(bool visible) {auto &o = get(); if (_Change(o.m_cursor, visible)) emit o.cursorVisibleChanged(o.m_cursor);}
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
    static bool isFullScreen() {return get().m_fullScreen;}
    static void setFullScreen(bool fs) { auto &o = get(); if (_Change(o.m_fullScreen, fs)) emit o.fullScreenChanged(o.m_fullScreen);}
    QString tr() const {return QString();}
    Q_INVOKABLE void registerItemToAcceptKey(QQuickItem *item);
    static QQuickItem *itemToAcceptKey() { return get().m_keyItems.isEmpty() ? nullptr : get().m_keyItems.front(); }
    static void setItemPressed(QQuickItem *item);
    static UtilObject &get() { if (!object) create(); return *object; }
//    static void setEngine(qmlEngine( *))
    static void setQmlEngine(QQmlEngine *engine) { get().m_engine = engine; }
    static QQmlEngine *qmlEngine() { return get().m_engine; }
signals:
    void trChanged();
    void mouseReleased(const QPointF &scenePos);
    void cursorVisibleChanged(bool cursorVisible);
    void fullScreenChanged(bool fullScreen);
private:
    UtilObject(QObject *parent = nullptr);
    static void create();
    void changeEvent(QEvent *event) {
        if (event->type() == QEvent::LanguageChange)
            emit trChanged();
    }
    QSet<QQuickItem*> m_itemsToAcceptKey;
    QLinkedList<QQuickItem*> m_keyItems;
    bool removeKeyItem(QQuickItem *item) {
        auto it = _Find(m_keyItems.begin(), m_keyItems.end(), item);
        if (it == m_keyItems.end())
            return false;
        m_keyItems.erase(it);
        return true;
    }
    bool m_fullScreen, m_cursor, m_filterDoubleClick;
    QQmlEngine *m_engine = nullptr;
    static UtilObject *object;
};

#endif // GLOBALQMLOBJECT_HPP
