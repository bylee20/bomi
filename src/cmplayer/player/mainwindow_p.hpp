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
#include "misc/downloader.hpp"
#include "video/videorendereritem.hpp"
#include "subtitle/subtitlerendereritem.hpp"
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

class ActionGroup;

struct EnumGroup {
    EnumGroup() {}
    EnumGroup(const char *p, ActionGroup *g)
        : property(p), group(g) { }
    const char *property = nullptr;
    ActionGroup *group = nullptr;
};

enum SnapshotMode {
    NoSnapshot, QuickSnapshot, QuickSnapshotNoSub, SnapshotTool
};

using MSig = Signal<MrlState>;

class SubtitleFindDialog;               class SnapshotDialog;
class PrefDialog;                       class SubtitleView;
class TrayIcon;

struct MainWindow::Data {
    Data(MainWindow *p);
    template<class T>
    SIA typeKey() -> QString { return _L(EnumInfo<T>::typeKey()); }

    MainWindow *p = nullptr;
    SnapshotMode snapshotMode = NoSnapshot;
    QList<EnumGroup> enumGroups;
    MainQuickView *view = nullptr;
    bool visible = false, sotChanging = false, fullScreen = false;
    QQuickItem *player = nullptr;
    RootMenu menu;
    RecentInfo recent;
    AppState as;
    PlayEngine engine;
    VideoRenderer vr;
    SubtitleRendererItem subtitle;
    QPoint prevPos;
    Qt::WindowStates winState = Qt::WindowNoState;
    Qt::WindowStates prevWinState = Qt::WindowNoState;
    Qt::MouseButton pressedButton = Qt::NoButton;
    bool moving = false, changingSub = false;
    bool pausedByHiding = false, dontShowMsg = true, dontPause = false;
    bool stateChanging = false, loading = false, sgInit = false;
    QTimer loadingTimer, hider, initializer;
    ABRepeatChecker ab;
    QMenu contextMenu;
    PrefDialog *prefDlg = nullptr;
    SubtitleFindDialog *subFindDlg = nullptr;
    SnapshotDialog *snapshot = nullptr;
    OpenGLLogger glLogger{"SG"};
    QStringList loadedSubtitleFiles;
    SubtitleView *subtitleView = nullptr;
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

    auto pref() const -> const Pref& {return preferences;}
    auto actionId(MouseBehavior mb, QInputEvent *event) const -> QString
        { return preferences.mouse_action_map[mb][event->modifiers()]; }
    auto setOpen(const Mrl &mrl) -> void
        { if (mrl.isLocalFile()) _SetLastOpenPath(mrl.toLocalFile()); }
    auto appendSubFiles(const QStringList &files, bool checked,
                        const QString &enc) -> void;
    auto clearSubtitleFiles() -> void;
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
    auto showMessage(const QString &cmd, bool value) -> void
        { showMessage(cmd, value ? tr("On") : tr("Off")); }
    auto doVisibleAction(bool visible) -> void;
    auto commitData() -> void;
    auto subtitleState() const -> SubtitleStateInfo;
    auto setSubtitleState(const SubtitleStateInfo &state) -> void;
    auto initWidget() -> void;
    auto generatePlaylist(const Mrl &mrl) const -> Playlist;
    auto openMrl(const Mrl &mrl) -> void;
    auto resume(const Mrl &mrl, int *edition) -> int;
    auto cache(const Mrl &mrl) -> int;
    auto initEngine() -> void;
    auto initItems() -> void;
    auto initTimers() -> void;
    auto connectMenus() -> void;
    auto setVideoSize(const QSize &video) -> void;
    auto syncState() -> void;
    auto syncWithState() -> void;
    auto load(const Mrl &mrl, bool play = true) -> void;
    auto load(Subtitle &sub, const QString &file, const QString &enc) -> bool;
    auto reloadSkin() -> void;
    auto trigger(QAction *action) -> void;
    auto updateSubtitleState() -> void;
    auto tryToAutoselect(const QVector<SubComp> &loaded,
                         const Mrl &mrl) -> QVector<int>;
    auto tryToAutoload(const Mrl &mrl,
                       const QString &path = QString()) -> QVector<SubComp>;
    auto cancelToHideCursor() -> void
        { hider.stop(); view->setCursorVisible(true); }
    auto readyToHideCursor() -> void;
    auto initContextMenu() -> void;
    auto openWith(const OpenMediaInfo &mode, const QList<Mrl> &mrls) -> void;
    auto lastCheckedSubtitleIndex() const -> int;
    auto setCurrentSubtitleIndexToEngine() -> void
        { engine.setCurrentSubtitleIndex(lastCheckedSubtitleIndex()); }
    auto setSubtitleTracksToEngine() -> void;
    auto syncSubtitleFileMenu() -> void;
    auto updateWindowSizeState() -> void;
    auto screenSize() const -> QSize;
    auto updateWindowPosState() -> void;
    auto checkWindowState(Qt::WindowStates prev) -> void;

    template<class List>
    auto updateListMenu(Menu &menu, const List &list,
                        int current, const QString &group = QString()) -> void;
    template<class F>
    auto plugCurrentStreamActions(Menu *menu, F f, QString g = QString()) -> void;
    template<class T, class Func>
    auto push(const T &to, const T &from, const Func &func) -> QUndoCommand*;
    auto showTimeLine() -> void;
    auto showMessageBox(const QVariant &msg) -> void;
    auto showOSD(const QVariant &msg) -> void;
    template<class T, class F, class Handler, class State>
    auto plugEnumActions(Menu &menu, State &state, Handler handler,
                            const char *asprop, Signal<State> sig, F f) -> void;
    template<class T, class F, class S>
    auto plugEnumActions(Menu &menu, S &s, const char *prop, Signal<S> sig, F f) -> void;
    template<class T, class F>
    auto plugEnumActions(Menu &menu, const char *asprop, MSig sig, F f) -> void
    { plugEnumActions<T, F, MrlState>(menu, as.state, asprop, sig, f); }

    template<class T, class F, class GetNew, class S>
    auto plugEnumDataActions(Menu &menu, S &s, const char *prop, GetNew getNew,
                             Signal<S> sig, F f) -> void;
    template<class T, class F, class GetNew>
    auto plugEnumDataActions(Menu &menu, const char *asprop, GetNew getNew,
                             MSig sig, F f) -> void
    { plugEnumDataActions<T>(menu, as.state, asprop, getNew, sig, f); }

    template<class T, class F, class S>
    auto plugEnumMenu(Menu &parent, S &s, const char *prop,
                      Signal<S> sig, F f) -> void
    { plugEnumActions<T, F>(parent(typeKey<T>()), s, prop, sig, f); }
    template<class T, class F>
    auto plugEnumMenu(Menu &parent, const char *asprop,
                      MSig sig, F f) -> void
    { plugEnumActions<T, F>(parent(typeKey<T>()), as.state, asprop, sig, f); }
    template<class F>
    auto plugStepActions(Menu &menu, const char *asprop, MSig sig, F f) -> void;
    template<class T, class F, class GetNew>
    auto plugPropertyDiff(ActionGroup *g, const char *asprop, GetNew getNew,
                          MSig sig, F f) -> void;
    template<class T, class F>
    auto plugPropertyDiff(ActionGroup *g, const char *asprop, T min, T max,
                          MSig sig, F f) -> void;
    template<class F>
    auto plugPropertyCheckable(QAction *a, const char *p, MSig s, F f) -> void;
    template<class F>
    auto plugStreamActions(Menu *m, F f, const QString &g = QString()) -> void;
};

#endif // MAINWINDOW_P_HPP
