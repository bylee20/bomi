#ifndef MAINWINDOW_P_HPP
#define MAINWINDOW_P_HPP

#include "mainwindow.hpp"
#include "appstate.hpp"
#include "rootmenu.hpp"
#include "recentinfo.hpp"
#include "abrepeatchecker.hpp"
#include "playengine.hpp"
#include "playlistmodel.hpp"
#include "historymodel.hpp"
#include "pref/pref.hpp"
#include "streamtrack.hpp"
#include "misc/downloader.hpp"
#include "misc/youtubedl.hpp"
#include "misc/yledl.hpp"
#include "video/videorenderer.hpp"
#include "subtitle/subtitlerenderer.hpp"
#include "opengl/opengllogger.hpp"
#include "quick/themeobject.hpp"
#include "quick/toplevelitem.hpp"
#include "quick/windowobject.hpp"
#include "misc/stepaction.hpp"
#include "misc/logviewer.hpp"
#include "os/os.hpp"
#include "misc/smbauth.hpp"
#include "misc/dataevent.hpp"
#include "json/jrserver.hpp"
#include "player/jrplayer.hpp"
#include <QUndoCommand>
#include <QMimeData>
#include <QQmlProperty>
#include <QQmlEngine>

#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

DECLARE_LOG_CONTEXT(Main)

enum MainWindowEvent {
    GetSmbAuth = QEvent::User + 1
};

template<class Func, class T>
class ValueCmd : public QUndoCommand {
public:
    ValueCmd(const T &to, const T &from, const Func &func)
        : to(to), from(from), func(func) { }
    auto redo() -> void final { func(to); }
    auto undo() -> void final { func(from); }
private:
    T to, from; Func  func;
};

enum SnapshotMode {
    NoSnapshot, QuickSnapshot, QuickSnapshotNoSub, SnapshotTool
};

using MSig = Signal<MrlState>;

class SubtitleFindDialog;               class SnapshotDialog;
class PrefDialog;                       class SubtitleViewer;
class TrayIcon;                         class AudioEqualizerDialog;
class IntrplDialog;                     class VideoColorDialog;
class EncoderDialog;                    class FileNameGenerator;

struct MainWindow::Data {
    Data(MainWindow *p): p(p) { }

    template<class T>
    SIA typeKey() -> QString { return _L(EnumInfo<T>::typeKey()); }

    MainWindow *p = nullptr;
    QQuickItem *player = nullptr, *cropbox = nullptr;
    RootMenu &menu = RootMenu::instance();
    RecentInfo recent;
    AppState as;
    PlayEngine &e = *p->m_engine;
    QPoint mouseStartPos, winStartPos;
    YouTubeDL youtube;
    YleDL yle;

#ifdef Q_OS_WIN
    QWinTaskbarButton taskbar;
#endif

    Qt::MouseButton pressedButton = Qt::NoButton;
    Qt::MouseButton contextMenuButton = Qt::RightButton;
    KeyModifier contextMenuModifier = KeyModifier::None;
    struct {
        QTimer timer; QAction *action;
        auto unset() { timer.stop(); action = nullptr; }
    } singleClick;
    bool moving = false;
    bool pausedByHiding = false;
    bool stateChanging = false;
    bool hidingCursorPended = true, noMessage = true;
    MouseObject *mouse = nullptr;

    struct {
        QDate date; QTime time, position;
        quint64 unix_ = 0;
        QMap<QString, std::function<QString(void)>> get;
    } ph;
    QTimer waiter, hider, dialogWorkaround;
    ABRepeatChecker ab;
    QMenu contextMenu;
    QSharedPointer<PrefDialog> prefDlg;
    QSharedPointer<SubtitleFindDialog> subFindDlg;
    QSharedPointer<LogViewer> logViewer;
    QSharedPointer<SnapshotDialog> snapshot;
    QSharedPointer<SubtitleViewer> sview;
    QSharedPointer<AudioEqualizerDialog> eq;
    QSharedPointer<VideoColorDialog> color;
    QSharedPointer<IntrplDialog> intrpl, chroma, intrplDown;
    QSharedPointer<EncoderDialog> encoder;
    PlaylistModel playlist;
    QUndoStack undo;
    Downloader downloader;
    TrayIcon *tray = nullptr;
    QString filePath;
    Pref pref;
    ThemeObject theme;
    QList<QAction*> unblockedActions;
    HistoryModel history;
    SnapshotMode snapshotMode = NoSnapshot;

    TopLevelItem *top = nullptr;
    OS::WindowAdapter *adapter = nullptr;
    JrServer *jrServer = nullptr;
    JrPlayer jrPlayer;
    Qt::WindowState prevWindowState = Qt::WindowNoState;
    int wheelAngles = 0;

    auto fileNameGenerator(const QTime &end = QTime()) const -> FileNameGenerator;

    template<class T, class... Args>
    auto dialog(const Args&... args) -> QSharedPointer<T>
    {
        auto dlg = new T(args...);
        dlg->winId();
        Q_ASSERT(p && dlg->windowHandle());
        dlg->windowHandle()->setTransientParent(p);
#ifdef Q_OS_WIN
        dlg->installEventFilter(p);
#endif
        return QSharedPointer<T>(dlg);
    }

    template <class T = QObject>
    auto findItem(const QString &name = QString()) -> T*
        { return p->rootObject()->findChild<T*>(name); }
    auto clear() -> void;
    auto resizeContainer() -> void;
    auto actionId(MouseBehavior mb, QInputEvent *event) const -> QString
        { return pref.mouse_action_map()[mb][event->modifiers()]; }
    auto setOpen(const Mrl &mrl) -> void
    {
        if (mrl.isLocalFile())
            _SetLastOpenPath(mrl.toLocalFile());
        playlist.setLoaded(mrl);
    }
    auto deleteDialogs() -> void;
    auto restoreState() -> void;
    auto applyPref() -> void;
    auto updateStaysOnTop() -> void;
    auto videoSize(const WindowSize &hint) -> QSize;
    auto setVideoSize(const QSize &video) -> void;
    auto updateRecentActions(const QList<Mrl> &list) -> void;
    auto updateMrl(const Mrl &mrl) -> void;
    auto updateTitle() -> void;
    auto showMessage(const QString &msg, const bool *force = nullptr) -> void;
    auto showMessage(const QString &cmd, const QString &desc) -> void
        { showMessage(cmd % u": "_q % desc); }
    static auto toMessage(bool value) -> QString { return value ? tr("On") : tr("Off"); }
    auto showMessage(const QString &cmd, bool value) -> void
        { showMessage(cmd, toMessage(value)); }
    auto doVisibleAction(bool visible) -> void;
    auto commitData() -> void;
    auto initWindow() -> void;
    auto initTray() -> void;
    auto generatePlaylist(const Mrl &mrl) const -> Playlist;
    auto openMrl(const Mrl &mrl) -> void;
    auto openMimeData(const QMimeData *md) -> void;
    auto plugEngine() -> void;
    auto initItems() -> void;
    auto plugMenu() -> void;
    auto load(const Mrl &mrl, bool play = true,
              bool tryResume = true, const QString &sub = QString()) -> void;
    auto reloadSkin() -> void;
    auto trigger(QAction *action) -> void;
    auto setCursorVisible(bool visible) -> void;
    auto cancelToHideCursor() -> void;
    auto readyToHideCursor() -> void;
    auto initContextMenu() -> void;
    auto openWith(const OpenMediaInfo &mode, const QList<Mrl> &mrls,
                  const QString &sub = QString()) -> void;
    auto openDir(const QString &dir = QString()) -> void;
    auto screenSize() const -> QSize;
    auto updateWaitingMessage() -> void;
    auto updateWindowState(Qt::WindowState ws) -> void;

    template<class T, class Func>
    auto push(const T &to, const T &from, const Func &func) -> QUndoCommand*;
    template<class T, class S>
    auto push(const T &to, const T &old, void(PlayEngine::*set)(S)) -> QUndoCommand*;
    template<class T, class S>
    auto push(const T &to, T(MrlState::*get)() const, void(PlayEngine::*set)(S)) -> QUndoCommand*;
    auto showTimeLine() -> void;
    auto showMessageBox(const QVariant &msg) -> void;
    auto showOSD(const QVariant &msg) -> void;

#define PLUG_MEMFUNC(T) T(MrlState::*get)() const, void(MrlState::*sig)(T), \
        QString(MrlState::*desc)() const, void(PlayEngine::*set)(T)
#define PLUG_HELPER(p, s) &MrlState::p, &MrlState::p##_changed, &MrlState::desc_##p, &PlayEngine::s
    auto plugFlag(QAction *action, PLUG_MEMFUNC(bool)) -> void;
#define PLUG_FLAG(a, p, s) plugFlag(a, PLUG_HELPER(p, s))
    template<class T>
    auto plugStep(ActionGroup *g, PLUG_MEMFUNC(T)) -> void;
#define PLUG_STEP(m, p, s) plugStep(m, PLUG_HELPER(p, s))
    template<class T>
    auto plugEnum(ActionGroup *g, PLUG_MEMFUNC(T), QAction *cycle) -> void;
    template<class T>
    auto plugEnum(Menu &m, PLUG_MEMFUNC(T)) -> void
    { plugEnum<T>(m.g(_L(EnumInfo<T>::typeKey())), get, sig, desc, set, m.action(u"cycle"_q)); }
#define PLUG_ENUM(m, p, s) plugEnum(m, PLUG_HELPER(p, s))
    template<class T>
    auto plugEnumChild(Menu &parent, PLUG_MEMFUNC(T)) -> void
    { plugEnum<T>(parent(_L(EnumInfo<T>::typeKey())), get, sig, desc, set); }
#define PLUG_ENUM_CHILD(pm, p, s) plugEnumChild(pm, PLUG_HELPER(p, s))
    template<class T>
    auto plugAppEnumChild(Menu &parent, const char *prop, void(AppState::*sig)(T)) -> void;
    auto plugTrack(Menu &parent, StreamType type, const QString &gkey = QString(), QAction *sep = nullptr) -> void;

    auto plugCycle(Menu &parent, const QString &g = QString()) -> void
    { plugCycle(parent, parent.g(g)); }
    auto plugCycle(Menu &parent, ActionGroup *g) -> void
    { plugCycle(parent[u"cycle"_q], g); }
    auto plugCycle(QAction *cycle, ActionGroup *g) -> void
    { if (cycle) connect(cycle, &QAction::triggered, [=] () { triggerNextAction(g->actions()); }); }

    template<class T, class F>
    auto propertyMessage(QString(MrlState::*desc)() const,
                         void(MrlState::*sig)(T), F func) -> void
        { connect(e.params(), sig, p, [=] (T val) { showMessage((e.params()->*desc)(), func(val)); }); }

    template<class T>
    auto propertyMessage(QString(MrlState::*desc)() const, void(MrlState::*sig)(T)) -> void
        { connect(e.params(), sig, p, [=] (T val) { showMessage((e.params()->*desc)(), val); }); }
#define PROP_NOTIFY(p, ...) propertyMessage(&MrlState::desc_##p, &MrlState::p##_changed, ##__VA_ARGS__)

    template<class T>
    auto propertyMessage(QString(MrlState::*desc)() const, void(MrlState::*sig)(T), StepValue Steps::*step) -> void
        { propertyMessage(desc, sig, [=] (T val) { return (pref.steps().*step).text(val); }); }
#define STEP_MESSAGE(p, s) propertyMessage(&MrlState::desc_##p, &MrlState::p##_changed, &Steps::s)

    static auto triggerNextAction(const QList<QAction*> &actions) -> void;
};

#endif // MAINWINDOW_P_HPP
