#ifndef MAINWINDOW_P_HPP
#define MAINWINDOW_P_HPP

#include "mainwindow.hpp"
#include "appstate.hpp"
#include "rootmenu.hpp"
#include "recentinfo.hpp"
#include "abrepeatchecker.hpp"
#include "playengine.hpp"
#include "mainquickview.hpp"
#include "playlistmodel.hpp"
#include "historymodel.hpp"
#include "pref.hpp"
#include "streamtrack.hpp"
#include "misc/downloader.hpp"
#include "misc/youtubedl.hpp"
#include "misc/yledl.hpp"
#include "video/videorenderer.hpp"
#include "subtitle/subtitlerenderer.hpp"
#include "opengl/opengllogger.hpp"
#include "quick/themeobject.hpp"
#include "misc/stepaction.hpp"

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
class PrefDialog;                       class SubtitleView;
class TrayIcon;                         class AudioEqualizerDialog;

struct MainWindow::Data {
    template<class T>
    SIA typeKey() -> QString { return _L(EnumInfo<T>::typeKey()); }

    MainWindow *p = nullptr;
    MainQuickView *view = nullptr;
    bool visible = false, sotChanging = false, fullScreen = false;
    bool starting = false;
    QQuickItem *player = nullptr;
    RootMenu menu;
    RecentInfo recent;
    AppState as;
    PlayEngine e;
    QPoint mouseStartPos, winStartPos;
    YouTubeDL youtube;
    YleDL yle;

    Qt::WindowStates winState = Qt::WindowNoState;
    Qt::WindowStates prevWinState = Qt::WindowNoState;
    Qt::MouseButton pressedButton = Qt::NoButton;
    bool moving = false, changingSub = false;
    bool pausedByHiding = false, dontShowMsg = true, dontPause = false;
    bool stateChanging = false, loading = false, sgInit = false;
    QTimer waiter, hider;
    ABRepeatChecker ab;
    QMenu contextMenu;
    PrefDialog *prefDlg = nullptr;
    SubtitleFindDialog *subFindDlg = nullptr;
    SnapshotDialog *snapshot = nullptr;
    OpenGLLogger glLogger{"SG"};
    SubtitleView *sview = nullptr;
    PlaylistModel playlist;
    QUndoStack *undo = nullptr;
    Downloader downloader;
    TrayIcon *tray = nullptr;
    QString filePath;
    Pref preferences;
    QAction *subtrackSep = nullptr;
    QDesktopWidget *desktop = nullptr;
    QSize virtualDesktopSize;
    ThemeObject theme;
    QList<QAction*> unblockedActions;
    HistoryModel history;
    SnapshotMode snapshotMode = NoSnapshot;
    AudioEqualizerDialog *eq = nullptr;

    auto pref() const -> const Pref& {return preferences;}
    auto actionId(MouseBehavior mb, QInputEvent *event) const -> QString
        { return preferences.mouse_action_map[mb][event->modifiers()]; }
    auto setOpen(const Mrl &mrl) -> void
    {
        if (mrl.isLocalFile())
            _SetLastOpenPath(mrl.toLocalFile());
        playlist.setLoaded(mrl);
    }
    auto applyPref() -> void;
    auto updateStaysOnTop() -> void;
    auto setVideoSize(double rate) -> void;
    auto updateRecentActions(const QList<Mrl> &list) -> void;
    auto updateMrl(const Mrl &mrl) -> void;
    auto updateTitle() -> void;
    auto showMessage(const QString &msg, const bool *force = nullptr) -> void;
    auto showMessage(const QString &cmd, int value, const QString &unit,
                     bool sign = false) -> void;
    auto showMessage(const QString &cmd, double value, const QString &unit,
                     bool sign = false) -> void;
    auto showMessage(const QString &cmd, const QString &desc) -> void
        { showMessage(cmd % u": "_q % desc); }
    static auto toMessage(bool value) -> QString { return value ? tr("On") : tr("Off"); }
    auto showMessage(const QString &cmd, bool value) -> void
        { showMessage(cmd, toMessage(value)); }
    auto doVisibleAction(bool visible) -> void;
    auto commitData() -> void;
    auto initWidget() -> void;
    auto generatePlaylist(const Mrl &mrl) const -> Playlist;
    auto openMrl(const Mrl &mrl) -> void;
    auto resume(const Mrl &mrl, int *edition) -> int;
    auto cache(const Mrl &mrl) -> int;
    auto initEngine() -> void;
    auto initItems() -> void;
    auto connectMenus() -> void;
    auto setVideoSize(const QSize &video) -> void;
    auto load(const Mrl &mrl, bool play = true, bool tryResume = true) -> void;
    auto load(Subtitle &sub, const QString &file, const QString &enc) -> bool;
    auto reloadSkin() -> void;
    auto trigger(QAction *action) -> void;
    auto tryToAutoselectMode(const QVector<SubComp> &loaded,
                         const Mrl &mrl) -> QVector<int>;
    auto cancelToHideCursor() -> void
        { hider.stop(); view->setCursorVisible(true); }
    auto readyToHideCursor() -> void;
    auto initContextMenu() -> void;
    auto openWith(const OpenMediaInfo &mode, const QList<Mrl> &mrls) -> void;
    auto openDir(const QString &dir = QString()) -> void;
    auto updateWindowSizeState() -> void;
    auto screenSize() const -> QSize;
    auto updateWindowPosState() -> void;
    auto checkWindowState(Qt::WindowStates prev) -> void;
    auto updateWaitingMessage() -> void;

    template<class T, class Func>
    auto push(const T &to, const T &from, const Func &func) -> QUndoCommand*;
    template<class T, class S, class ToString>
    auto push(const T &to, const char *p, T(MrlState::*get)() const,
              void(PlayEngine::*set)(S), ToString ts) -> QUndoCommand*;
    auto showTimeLine() -> void;
    auto showMessageBox(const QVariant &msg) -> void;
    auto showOSD(const QVariant &msg) -> void;
    template<class T>
    auto showProperty(const char *p, T(MrlState::*get)() const) -> void
        { showProperty(p, (e.params()->*get)()); }
    template<class T>
    auto showProperty(const char *p, const T &val) -> void
        { showMessage(e.params()->description(p), val); }

    auto plugFlag(QAction *action, const char *property,
                  bool(MrlState::*get)() const, void(MrlState::*sig)(bool),
                  void(PlayEngine::*set)(bool)) -> void;
#define PLUG_FLAG(a, p, s) plugFlag(a, #p, &MrlState::p, &MrlState::p##_changed, &PlayEngine::s)
    auto plugStep(ActionGroup *g, const char *prop, int(MrlState::*get)() const,
                  void(PlayEngine::*set)(int)) -> void;
#define PLUG_STEP(m, p, s) plugStep(m, #p, &MrlState::p, &PlayEngine::s)
    template<class T>
    auto plugEnum(ActionGroup *g, const char *prop, T(MrlState::*get)() const,
                  void(MrlState::*sig)(T), void(PlayEngine::*set)(T), QAction *cycle) -> void;
    template<class T>
    auto plugEnum(Menu &m, const char *property, T(MrlState::*get)() const,
                  void(MrlState::*sig)(T), void(PlayEngine::*set)(T)) -> void
    { plugEnum<T>(m.g(_L(EnumInfo<T>::typeKey())), property, get, sig, set, m.action(u"cycle"_q)); }
#define PLUG_ENUM(m, p, s) plugEnum(m, #p, &MrlState::p, &MrlState::p##_changed, &PlayEngine::s)
    template<class T>
    auto plugEnumChild(Menu &parent, const char *property, T(MrlState::*get)() const,
                       void(MrlState::*sig)(T), void(PlayEngine::*set)(T)) -> void
    { plugEnum<T>(parent(_L(EnumInfo<T>::typeKey())), property, get, sig, set); }
#define PLUG_ENUM_CHILD(pm, p, s) plugEnumChild(pm, #p, &MrlState::p, &MrlState::p##_changed, &PlayEngine::s)
    template<class T>
    auto plugAppEnumChild(Menu &parent, const char *prop, void(AppState::*sig)(T)) -> void;
    auto plugTrack(Menu &parent, void(MrlState::*sig)(StreamList),
                   void(PlayEngine::*set)(int,bool), const QString &gkey = QString(),
                   QAction *sep = nullptr) -> void;
    static auto triggerNextAction(const QList<QAction*> &actions) -> void;
};

#endif // MAINWINDOW_P_HPP
