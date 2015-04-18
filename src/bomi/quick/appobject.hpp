#ifndef APPOBJECT_HPP
#define APPOBJECT_HPP

#include <QQuickItem>

class PlayEngine;                       class HistoryModel;
class PlaylistModel;                    class TopLevelItem;
class Downloader;                       class ThemeObject;
class WindowObject;                     class MainWindow;

class MemoryObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal total READ total CONSTANT FINAL)
    Q_PROPERTY(qreal usage READ usage NOTIFY usageChanged)
public:
    MemoryObject();
    ~MemoryObject();
    auto total() const -> qreal { return m_total; }
    auto usage() const -> qreal { return m_usage; }
signals:
    void usageChanged();
private:
    qreal m_total = 1, m_usage = 0;
    QTimer m_timer;
};

class CpuObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal usage READ usage NOTIFY usageChanged)
    Q_PROPERTY(int cores READ cores CONSTANT FINAL)
public:
    CpuObject();
    ~CpuObject();
    auto usage() const -> qreal { return m_usage; }
    auto cores() const -> int { return m_cores; }
signals:
    void usageChanged();
private:
    qreal m_usage = 0;
    quint64 m_pt = 0, m_st = 0;
    int m_cores = 1;
    QTimer m_timer;
};

class AppObject : public QObject {
    Q_OBJECT
    Q_ENUMS(Event)
    Q_FLAGS(Events)
    Q_PROPERTY(PlayEngine *engine READ engine CONSTANT FINAL)
    Q_PROPERTY(HistoryModel *history READ history CONSTANT FINAL)
    Q_PROPERTY(PlaylistModel *playlist READ playlist CONSTANT FINAL)
    Q_PROPERTY(TopLevelItem *topLevelItem READ topLevelItem CONSTANT FINAL)
    Q_PROPERTY(Downloader *download READ downloader CONSTANT FINAL)
    Q_PROPERTY(ThemeObject *theme READ theme CONSTANT FINAL)
    Q_PROPERTY(WindowObject *window READ window CONSTANT FINAL)
    Q_PROPERTY(MemoryObject *memory READ memory CONSTANT FINAL)
    Q_PROPERTY(CpuObject *cpu READ cpu CONSTANT FINAL)
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)
public:
    static const int MouseEvent = 0x1000;
    enum Event {
        NoEvent = 0,
        KeyEvent = 1,
        DoubleClickEvent = 2 | MouseEvent
    };
    Q_DECLARE_FLAGS(Events, Event)
    auto engine() const -> PlayEngine* { return s.engine; }
    auto history() const -> HistoryModel* { return s.history; }
    auto playlist() const -> PlaylistModel* { return s.playlist; }
    auto topLevelItem() const -> TopLevelItem* { return s.top; }
    auto downloader() const -> Downloader* { return s.down; }
    auto theme() const -> ThemeObject* { return s.theme; }
    auto window() const -> WindowObject* { return s.window; }
    auto memory() const -> MemoryObject* { return &m_memory; }
    auto cpu() const -> CpuObject* { return &m_cpu; }
    auto displayName() const -> QString;
    Q_INVOKABLE void registerToAccept(QQuickItem *item, Events e);
    Q_INVOKABLE QString description(const QString &actionId) const;
    Q_INVOKABLE double textWidth(const QString &text, int size) const;
    Q_INVOKABLE double textWidth(const QString &text, int size, const QString &family) const;
    Q_INVOKABLE bool execute(const QString &id) const;
    Q_INVOKABLE QObject *action(const QString &id) const;
    Q_INVOKABLE void open(const QString &location);
    Q_INVOKABLE void delete_(QObject *o);

    static auto setTheme(ThemeObject *theme) -> void { s.theme = theme; }
    static auto setEngine(PlayEngine *engine) -> void { s.engine = engine; }
    static auto setHistory(HistoryModel *history) -> void { s.history = history; }
    static auto setPlaylist(PlaylistModel *pl) -> void { s.playlist = pl; }
    static auto setTopLevelItem(TopLevelItem *top) -> void { s.top = top; }
    static auto setDownloader(Downloader *down) -> void { s.down = down; }
    static auto setWindow(MainWindow *window) -> void;
    static auto setQmlEngine(QQmlEngine *engine) -> void { s.qml = engine; }
    static auto qmlEngine() -> QQmlEngine* { return s.qml; }
    static auto itemToAccept(Event event, const QPointF &scenePos = QPointF()) -> QQuickItem*;

    static auto dumpInfo() -> void;
signals:
    void displayNameChanged();
private:
    struct StaticData {
        PlayEngine *engine = nullptr;
        HistoryModel *history = nullptr;
        PlaylistModel *playlist = nullptr;
        TopLevelItem *top = nullptr;
        Downloader *down = nullptr;
        ThemeObject *theme = nullptr;
        WindowObject *window = nullptr;
        QHash<QQuickItem*, Events> itemsToAccept;
        QLinkedList<QQuickItem*> orderToAccept;
        QQmlEngine *qml = nullptr;
        MainWindow *mw = nullptr;
    };
    static StaticData s;
    mutable MemoryObject m_memory;
    mutable CpuObject m_cpu;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AppObject::Events)

#endif // APPOBJECT_HPP
