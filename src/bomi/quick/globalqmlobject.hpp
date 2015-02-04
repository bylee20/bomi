#ifndef GLOBALQMLOBJECT_HPP
#define GLOBALQMLOBJECT_HPP

class MainWindow;

enum MemoryUnit {
    ByteUnit = 1,
    Kilobyte = 1024,
    Megabyte = 1024*1024,
    Gigabyte = 1024*1024*1024
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
    Q_INVOKABLE static double textWidth(const QString &text, int size,
                                        const QString &family);
    Q_INVOKABLE QString msecToString(int ms);
    Q_INVOKABLE QString secToString(int s);
    Q_INVOKABLE QPointF mousePos(QQuickItem *item);
    Q_INVOKABLE QPointF mapFromSceneTo(QQuickItem *item,
                                       const QPointF &scenePos) const;
    Q_INVOKABLE bool execute(const QString &key);
    Q_INVOKABLE static QObject *action(const QString &key);
    Q_INVOKABLE double bound(double min, double v, double max) const;
    Q_INVOKABLE void showToolTip(QQuickItem *item, const QPointF &pos,
                                 const QString &text);
    Q_INVOKABLE void hideToolTip() { QToolTip::hideText(); }
    Q_INVOKABLE void filterDoubleClick() { m_filterDoubleClick = true; }
    Q_INVOKABLE void registerItemToAcceptKey(QQuickItem *item);
    Q_INVOKABLE bool containsMouse(QQuickItem *item);
    static auto isDoubleClickFiltered() -> bool;
    static auto resetFilterDoubleClick() -> void;
    static auto isCursorVisible() -> bool {return get().m_cursor;}
    static auto setCursorVisible(bool visible) -> void;
    static auto totalMemory(MemoryUnit unit) -> double;
    static auto usingMemory(MemoryUnit unit) -> double;
    static auto totalMemory() -> double { return totalMemory(Megabyte); }
    static auto usingMemory() -> double { return usingMemory(Megabyte); }
    static auto cpuUsage() -> double;
    static auto cores() -> int;
    static auto systemTime() -> quint64 { return _SystemTime(); }
    static auto processTime() -> quint64; // usec
    static auto monospace() -> QString;
    static auto isFullScreen() -> bool;
    static auto tr() -> QString {return QString();}
    static auto itemToAcceptKey() -> QQuickItem*;
    static auto get() -> UtilObject& { if (!object) create(); return *object; }
    static auto setQmlEngine(QQmlEngine *engine) -> void;
    static auto qmlEngine() -> QQmlEngine* { return get().m_engine; }
    static auto setMainWindow(MainWindow *mw) -> void;
signals:
    void trChanged();
    void mouseReleased(const QPointF &scenePos);
    void cursorVisibleChanged(bool cursorVisible);
    void fullScreenChanged(bool fullScreen);
private:
    UtilObject(QObject *parent = nullptr);
    auto removeKeyItem(QQuickItem *item) -> bool;
    static auto create() -> void;
    QSet<QQuickItem*> m_itemsToAcceptKey;
    QLinkedList<QQuickItem*> m_keyItems;
    bool m_cursor, m_filterDoubleClick;
    QQmlEngine *m_engine = nullptr;
    MainWindow *m_main = nullptr;
    static UtilObject *object;
};

inline auto UtilObject::msecToString(int ms) -> QString
{ return secToString(qRound((double)ms*1e-3)); }

inline auto UtilObject::secToString(int s) -> QString
{ return _MSecToTime(s*1000).toString(u"h:mm:ss"_q); }

inline auto UtilObject::bound(double min, double v, double max) const -> double
{return qBound(min, v, max);}

inline auto UtilObject::showToolTip(QQuickItem *item, const QPointF &pos,
                                    const QString &text) -> void
{
    const auto p = item->mapToScene(pos).toPoint();
    QToolTip::showText(item->window()->mapToGlobal(p), text);
}

inline auto UtilObject::isDoubleClickFiltered() -> bool
{ return get().m_filterDoubleClick; }

inline auto UtilObject::resetFilterDoubleClick() -> void
{ get().m_filterDoubleClick = false; }

inline auto UtilObject::setCursorVisible(bool visible) -> void
{
    auto &self = get();
    if (_Change(self.m_cursor, visible))
        emit self.cursorVisibleChanged(self.m_cursor);
}

inline auto UtilObject::itemToAcceptKey() -> QQuickItem*
{ return get().m_keyItems.isEmpty() ? nullptr : get().m_keyItems.front(); }

inline auto UtilObject::setQmlEngine(QQmlEngine *engine) -> void
{ get().m_engine = engine; }

inline auto UtilObject::removeKeyItem(QQuickItem *item) -> bool
{
    auto it = std::find(m_keyItems.begin(), m_keyItems.end(), item);
    if (it == m_keyItems.end())
        return false;
    m_keyItems.erase(it);
    return true;
}
#endif // GLOBALQMLOBJECT_HPP
