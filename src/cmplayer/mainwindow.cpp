#include "stdafx.hpp"
#include "downloader.hpp"
#include "openmediafolderdialog.hpp"
#include "snapshotdialog.hpp"
#include "mainwindow.hpp"
#include "rootmenu.hpp"
#include "recentinfo.hpp"
#include "subtitlefinddialog.hpp"
#include "pref.hpp"
#include "abrepeater.hpp"
#include "playlistview.hpp"
#include "playlistmodel.hpp"
#include "playengine.hpp"
#include "appstate.hpp"
#include "historymodel.hpp"
#include "video/videorendereritem.hpp"
#include "subtitle/subtitlerendereritem.hpp"
#include "info.hpp"
#include "prefdialog.hpp"
#include "app.hpp"
#include "globalqmlobject.hpp"
#include "subtitle/subtitleview.hpp"
#include <functional>
#include "playlistmodel.hpp"
#include "translator.hpp"
#include "trayicon.hpp"
#include "playlist.hpp"
#include "subtitle/subtitle_parser.hpp"
#include "subtitle/subtitlemodel.hpp"
#include "opengl/openglcompat.hpp"
#include "video/videoformat.hpp"
#include "dataevent.hpp"
#include "quick/toplevelitem.hpp"
#include "quick/appobject.hpp"
#include "log.hpp"
#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

//DECLARE_LOG_CONTEXT(Main)

extern void initialize_vdpau_interop(QOpenGLContext *context);
extern void finalize_vdpau_interop(QOpenGLContext *context);

template<typename Func, typename T>
class ValueCmd : public QUndoCommand {
public:
    ValueCmd(const T &to, const T &from, const Func &func): to(to), from(from), func(func) { }
    void redo() { func(to); }
    void undo() { func(from); }
private:
    T to, from; Func  func;
};

struct MainWindow::Data {
    struct EnumGroup {
        EnumGroup() {}
        EnumGroup(const char *p, ActionGroup *g): property(p), group(g) {}
        const char *property = nullptr;
        ActionGroup *group = nullptr;
    };

    Data(MainWindow *p): p(p) {preferences.load();}
    QList<EnumGroup> enumGroups;
    MainView *view = nullptr;
    MainWindow *p = nullptr;
    bool visible = false, sotChanging = false, fullScreen = false;
    QQuickItem *player = nullptr;
    RootMenu menu;    RecentInfo recent;
    AppState &as = AppState::get();
    PlayEngine engine;
    VideoRendererItem renderer;
    SubtitleRendererItem subtitle;
    QPoint prevPos;
    Qt::WindowStates winState = Qt::WindowNoState, prevWinState = Qt::WindowNoState;
    bool middleClicked = false, moving = false, changingSub = false;
    bool pausedByHiding = false, dontShowMsg = true, dontPause = false;
    bool stateChanging = false, loading = false;
    QTimer loadingTimer, hider, initializer;
    TopLevelItem topLevelItem;
    ABRepeater ab = {&engine, &subtitle};
    QMenu contextMenu;
    PrefDialog *prefDlg = nullptr;
    SubtitleFindDialog *subFindDlg = nullptr;
    QStringList loadedSubtitleFiles;
    SubtitleView *subtitleView = nullptr;
    PlaylistModel playlist;
    QUndoStack *undo = nullptr;
    Downloader downloader;
//    FavoritesView *favorite;
    TrayIcon *tray = nullptr;
    QString filePath;
    Pref preferences;
    const Pref &pref() const {return preferences;}
    QAction *subtrackSep = nullptr;
    QDesktopWidget *desktop = nullptr;
    QSize virtualDesktopSize;

    void syncState() {
        for (auto &eg : enumGroups) {
            Q_ASSERT(as.state.property(eg.property).isValid());
            auto data = eg.group->data();
            if (data.isValid())
                as.state.setProperty(eg.property, data);
        }
    }
    void syncWithState() {
        for (auto &eg : enumGroups)
            eg.group->setChecked(as.state.property(eg.property), true);
    }

    void load(const Mrl &mrl, bool play = true) {
        StartInfo info;
        info.mrl = mrl;
        if (play) {
            if (info.mrl.hash().isEmpty() && info.mrl.isDisc())
                info.mrl.updateHash();
            info.resume = resume(info.mrl, &info.edition);
            info.cache = cache(info.mrl);
        }
        engine.load(info);
    }
    QList<QAction*> unblockedActions;
    void trigger(QAction *action) {
        if (!action)
            return;
        if (topLevelItem.isVisible()) {
            if (unblockedActions.isEmpty()) {
                unblockedActions += menu("window").actions();
                qSort(unblockedActions);
            }
            const auto it = qBinaryFind(_C(unblockedActions), action);
            if (it == unblockedActions.cend())
                return;
        }
        action->trigger();
    }

    void updateSubtitleState() {
        const auto &mrl = as.state.mrl;
        if (mrl.isLocalFile()) {
            const auto &p = preferences;
            const QFileInfo file(mrl.toLocalFile());
            auto autoselection = [this, &file, &p] (const QList<SubComp> &loaded) {
                QList<int> selected;
                if (loaded.isEmpty() || !p.sub_enable_autoselect)
                    return selected;
                QSet<QString> langSet;
                const QString base = file.completeBaseName();
                for (int i=0; i<loaded.size(); ++i) {
                    bool select = false;
                    if (p.sub_autoselect == SubtitleAutoselect::Matched) {
                        select = QFileInfo(loaded[i].fileName()).completeBaseName() == base;
                    } else if (p.sub_autoselect == SubtitleAutoselect::All) {
                        select = true;
                    } else if (p.sub_autoselect == SubtitleAutoselect::EachLanguage) {
            //            const QString lang = loaded[i].m_comp.language().id();
                        const QString lang = loaded[i].language();
                        if ((select = (!langSet.contains(lang))))
                            langSet.insert(lang);
                    }
                    if (select)
                        selected.append(i);
                }
                if (p.sub_autoselect == SubtitleAutoselect::Matched
                        && !selected.isEmpty() && !p.sub_ext.isEmpty()) {
                    for (int i=0; i<selected.size(); ++i) {
                        const QString fileName = loaded[selected[i]].fileName();
                        const QString suffix = QFileInfo(fileName).suffix().toLower();
                        if (p.sub_ext == suffix) {
                            const int idx = selected[i];
                            selected.clear();
                            selected.append(idx);
                            break;
                        }
                    }
                }
                return selected;
            };
            auto autoload = [this, mrl, &autoselection, &p](bool autoselect) {
                QList<SubComp> loaded;
                if (!p.sub_enable_autoload)
                    return loaded;
                const QStringList filter = Info::subtitleNameFilter();
                const QFileInfo fileInfo(mrl.toLocalFile());
                const QFileInfoList all = fileInfo.dir().entryInfoList(filter, QDir::Files, QDir::Name);
                const QString base = fileInfo.completeBaseName();
                for (int i=0; i<all.size(); ++i) {
                    if (p.sub_autoload != SubtitleAutoload::Folder) {
                        if (p.sub_autoload == SubtitleAutoload::Matched) {
                            if (base != all[i].completeBaseName())
                                continue;
                        } else if (!all[i].fileName().contains(base))
                            continue;
                    }
                    Subtitle sub;
                    if (this->p->load(sub, all[i].absoluteFilePath(), p.sub_enc)) {
                        for (int i=0; i<sub.size(); ++i)
                            loaded.push_back(sub[i]);
                    }
                }
                if (autoselect) {
                    const QList<int> selected = autoselection(loaded);
                    for (int i=0; i<selected.size(); ++i)
                        loaded[selected[i]].selection() = true;
                }
                return loaded;
            };
            subtitle.setComponents(autoload(true));
        } else
            this->p->clearSubtitleFiles();
        syncSubtitleFileMenu();
    }

    void setCursorVisible(bool visible) {
        if (visible && view->cursor().shape() == Qt::BlankCursor) {
            view->unsetCursor();
            UtilObject::setCursorVisible(true);
        } else if (!visible && view->cursor().shape() != Qt::BlankCursor) {
            view->setCursor(Qt::BlankCursor);
            UtilObject::setCursorVisible(false);
        }
    }

    void cancelToHideCursor() {
        hider.stop();
        setCursorVisible(true);
    }

    void readyToHideCursor() {
        if (pref().hide_cursor && (p->isFullScreen() || !pref().hide_cursor_fs_only))
            hider.start(pref().hide_cursor_delay);
        else
            cancelToHideCursor();
    }

    void initContextMenu() {
        auto d = this;
        auto addContextMenu = [d] (Menu &menu) {d->contextMenu.addMenu(menu.copied(&d->contextMenu));};
        addContextMenu(d->menu("open"));
        d->contextMenu.addSeparator();
        addContextMenu(d->menu("play"));
        addContextMenu(d->menu("video"));
        addContextMenu(d->menu("audio"));
        addContextMenu(d->menu("subtitle"));
        d->contextMenu.addSeparator();
        addContextMenu(d->menu("tool"));
        addContextMenu(d->menu("window"));
        d->contextMenu.addSeparator();
        d->contextMenu.addAction(d->menu("help")["about"]);
        d->contextMenu.addAction(d->menu["exit"]);
#ifdef Q_OS_MAC
    ////    qt_mac_set_dock_menu(&d->menu);
        QMenuBar *mb = cApp.globalMenuBar();
        qDeleteAll(mb->actions());
        auto addMenuBar = [this, mb] (Menu &menu) {mb->addMenu(menu.copied(mb));};
        addMenuBar(d->menu("open"));
        addMenuBar(d->menu("play"));
        addMenuBar(d->menu("video"));
        addMenuBar(d->menu("audio"));
        addMenuBar(d->menu("subtitle"));
        addMenuBar(d->menu("tool"));
        addMenuBar(d->menu("window"));
        addMenuBar(d->menu("help"));
#endif
    }

    void openWith(const Pref::OpenMedia &mode, const QList<Mrl> &mrls) {
        if (mrls.isEmpty())
            return;
        const auto mrl = mrls.first();
        auto d = this;
        auto checkAndPlay = [this] (const Mrl &mrl) {
            if (mrl != engine.mrl())
                return false;
            if (!engine.isPlaying())
                load(mrl);
            return true;
        };
        if (!checkAndPlay(mrl)) {
            Playlist playlist;
            if (mode.playlist_behavior == PlaylistBehaviorWhenOpenMedia::AppendToPlaylist) {
                d->playlist.append(mrls);
            } else if (mode.playlist_behavior == PlaylistBehaviorWhenOpenMedia::ClearAndAppendToPlaylist) {
                d->playlist.clear();
                playlist.append(mrls);
            } else if (mode.playlist_behavior == PlaylistBehaviorWhenOpenMedia::ClearAndGenerateNewPlaylist) {
                d->playlist.clear();
                playlist = p->generatePlaylist(mrl);
            }
            d->playlist.merge(playlist);
            d->load(mrl, mode.start_playback);
            if (!mrl.isDvd())
                d->recent.stack(mrl);
        }
        if (!p->isVisible())
            p->show();
    }

    int lastCheckedSubtitleIndex() const {
        auto &list = menu("subtitle")("track");
        const auto internal = list.g("internal")->actions();
        const auto external = list.g("external")->actions();
        for (int i = external.size()-1; i >= 0; --i) {
            if (external[i]->isChecked())
                return internal.size() + i;
        }
        for (int i = internal.size()-1; i >= 0; --i) {
            if (internal[i]->isChecked())
                return i;
        }
        return -1;
    }
    void setCurrentSubtitleIndexToEngine() {
        engine.setCurrentSubtitleIndex(lastCheckedSubtitleIndex());
    }
    void setSubtitleTracksToEngine() {
        auto &list = menu("subtitle")("track");
        const auto internal = list.g("internal")->actions();
        const auto external = list.g("external")->actions();
        QStringList tracks; tracks.reserve(internal.size() + external.size());
        for (int i=0; i<internal.size(); ++i)
            tracks.append(internal[i]->text());
        for (auto action : external)
            tracks.append(action->text());
        engine.setSubtitleTracks(tracks);
    }

    void syncSubtitleFileMenu() {
        if (changingSub)
            return;
        changingSub = true;
        auto &list = menu("subtitle")("track");
        auto g = list.g("external");
        const auto components = subtitle.components();
        while (g->actions().size() < components.size()) {
            auto action = g->addAction("");
            action->setCheckable(true);
            list.insertAction(subtrackSep, action);
        }
        while (g->actions().size() > components.size())
            delete g->actions().last();
        const auto actions = g->actions();
        Q_ASSERT(components.size() == actions.size());
        for (int i=0; i<actions.size(); ++i) {
            actions[i]->setText(components[i]->name());
            actions[i]->setData(i);
            actions[i]->setChecked(components[i]->selection());
        }
        list.syncActions();
        changingSub = false;
    }

    void updateWindowSizeState() const {
        if (!p->isFullScreen() && !p->isMinimized() && p->isVisible())
            AppState::get().win_size = p->size();
    }
    QSize screenSize() const {
        return desktop->isVirtualDesktop() ? virtualDesktopSize : desktop->availableGeometry(p).size();
    }
    void updateWindowPosState() const {
        if (!p->isFullScreen() && !p->isMinimized() && p->isVisible()) {
            auto &as = AppState::get();
            const auto screen = screenSize();
            as.win_pos.rx() = qBound(0.0, (double)p->x()/(double)screen.width(), 1.0);
            as.win_pos.ry() = qBound(0.0, (double)p->y()/(double)screen.height(), 1.0);
        }
    }
    template<typename List>
    void updateListMenu(Menu &menu, const List &list, int current, const QString &group = "") {
        if (group.isEmpty())
            menu.setEnabledSync(!list.isEmpty());
        if (!list.isEmpty()) {
            menu.g(group)->clear();
            for (typename List::const_iterator it = list.begin(); it != list.end(); ++it) {
                auto act = menu.addActionToGroupWithoutKey(it->name(), true, group);
                act->setData(it->id()); if (current == it->id()) act->setChecked(true);
            }
        } else if (!group.isEmpty()) // partial in menu
            menu.g(group)->clear();
        menu.syncActions();
    }
    template<typename F>
    void connectCurrentStreamActions(Menu *menu, F func, QString group = "") {
        auto checkCurrentStreamAction = [this, func, menu, group] () {
            const auto current = (engine.*func)();
            for (auto action : menu->g(group)->actions()) {
                if (action->data().toInt() == current) {
                    action->setChecked(true);
                    break;
                }
            }
        };
        MainWindow::connect(menu, &QMenu::aboutToShow, checkCurrentStreamAction);
        for (auto copy : menu->copies())
            MainWindow::connect(copy, &QMenu::aboutToShow, checkCurrentStreamAction);
    }
    template <typename T = QObject>
    T* findItem(const QString &name = QString()) {
        return player ? view->rootObject()->findChild<T*>(name) : nullptr;
    }

    void commitData() {
        static bool first = true;
        if (first) {
            recent.setLastPlaylist(playlist.playlist());
            recent.setLastMrl(engine.mrl());
            engine.shutdown();
            auto &as = AppState::get();
            if (!p->isFullScreen())
                updateWindowPosState();
            as.state.video_effects = renderer.effects();
            as.playlist_visible = playlist.isVisible();
            as.history_visible = history.isVisible();
            as.save();
            syncState();
            history.setAppState(&as.state);

            engine.waitUntilTerminated();
            cApp.processEvents();
            first = false;
        }
    }

    template<typename T, typename Func>
    QUndoCommand *push(const T &to, const T &from, const Func &func) {
        if (undo) {
            auto cmd = new ValueCmd<Func, T>(to, from, func);
            undo->push(cmd);
            return cmd;
        } else {
            func(to);
            return nullptr;
        }
    }
    void showTimeLine() {
        if (player)
            QMetaObject::invokeMethod(player, "showTimeLine");
    }
    void showMessageBox(const QVariant &msg) {
        if (player)
            QMetaObject::invokeMethod(player, "showMessageBox", Q_ARG(QVariant, msg));
    }
    void showOSD(const QVariant &msg) {
        if (player)
            QMetaObject::invokeMethod(player, "showOSD", Q_ARG(QVariant, msg));
    }

    template<typename T, typename F, typename OnTriggered, typename State>
    void connectEnumActions(State &state, Menu &menu, OnTriggered onTriggered, const char *asprop, void(State::*sig)(), F f) {
        Q_ASSERT(state.property(asprop).isValid());
        const int size = EnumInfo<T>::size();
        auto cycle = _C(menu)[size > 2 ? "next" : "toggle"];
        auto group = menu.g(_L(EnumInfo<T>::typeKey()));
        if (cycle) {
            connect(cycle, &QAction::triggered, p, [this, group] () {
                auto actions = group->actions();
                int i = 0;
                for (; i<actions.size(); ++i)
                    if (actions[i]->isChecked())
                        break;
                const int prev = i++;
                forever {
                    if (i >= actions.size())
                        i = 0;
                    if (i == prev)
                        break;
                    if (actions[i]->isEnabled())
                        break;
                    ++i;
                }
                if (i != prev)
                    actions[i]->trigger();
            });
        }
        connect(group, &QActionGroup::triggered, onTriggered);
        connect(&state, sig, f);
        group->trigger(state.property(asprop));
        emit (state.*sig)();
        if (std::is_same<State, MrlState>::value)
            enumGroups.append(EnumGroup{asprop, group});
    }
    template<typename T, typename F, typename State>
    void connectEnumActions(State &state, Menu &menu, const char *asprop, void(State::*sig)(), F f) {
        auto group = menu.g(_L(EnumInfo<T>::typeKey()));
        connectEnumActions<T, F>(state, menu, [this, asprop, group, &state, &menu] (QAction *a) {
            auto action = static_cast<EnumAction<T>*>(a);
            auto t = action->enum_();
            auto current = state.property(asprop).template value<T>();
            if (current != t)
                push<T>(t, current, [this, asprop, group, &state, &menu] (T t) {
                    if (auto action = group->find(t)) {
                        action->setChecked(true);
                        p->showMessage(menu.title(), action->text());
                        state.setProperty(asprop, action->data());
                    }
                });
        }, asprop, sig, f);
    }
    template<typename T, typename F>
    void connectEnumActions(Menu &menu, const char *asprop, void(MrlState::*sig)(), F f) {
        connectEnumActions<T, F, MrlState>(as.state, menu, asprop, sig, f);
    }

    template<typename T, typename F, typename GetNew, typename State>
    void connectEnumDataActions(State &state, Menu &menu, const char *asprop, GetNew getNew, void(State::*sig)(), F f) {
        using Data = typename EnumInfo<T>::Data;
        connectEnumActions<T, F>(state, menu, [this, asprop, getNew, &state, &menu] (QAction *a) {
            auto action = static_cast<EnumAction<T>*>(a);
            auto old = state.property(asprop).template value<Data>();
            auto new_ = getNew(action->data());
            auto step = dynamic_cast<StepAction*>(action);
            if (step)
                new_ = step->clamp(new_);
            if (old != new_)
                push<Data>(new_, old, [this, asprop, &state, &menu, step] (const Data &data) {
                    state.setProperty(asprop, QVariant::fromValue<Data>(data));
                    if (step)
                        p->showMessage(menu.title(), step->format(data));
                });
        }, asprop, sig, f);
    }
    template<typename T, typename F, typename GetNew>
    void connectEnumDataActions(Menu &menu, const char *asprop, GetNew getNew, void(MrlState::*sig)(), F f) {
        connectEnumDataActions<T, F, GetNew, MrlState>(as.state, menu, asprop, getNew, sig, f);
    }

    template<typename T, typename F, typename State>
    void connectEnumMenu(State &state, Menu &parent, const char *asprop, void(State::*sig)(), F f) {
        connectEnumActions<T, F>(state, parent(EnumInfo<T>::typeKey()), asprop, sig, f);
    }

    template<typename T, typename F>
    void connectEnumMenu(Menu &parent, const char *asprop, void(MrlState::*sig)(), F f) {
        connectEnumActions<T, F>(as.state, parent(EnumInfo<T>::typeKey()), asprop, sig, f);
    }
    template<typename F>
    void connectStepActions(Menu &menu, const char *asprop, void(MrlState::*sig)(), F f) {
        connectEnumDataActions<ChangeValue, F>(menu, asprop, [this, asprop] (int diff) {
            return diff ? (as.state.property(asprop).toInt() + diff) : 0;
        }, sig, f);
    }

    SubtitleStateInfo subtitleState() const {
        SubtitleStateInfo state;
        state.track() = engine.currentSubtitleStream();
        state.mpv() = engine.subtitleFiles();
        const auto parsed = subtitle.components();
        for (auto c : parsed)
            state.append(*c);
        return state;
    }
    void setSubtitleState(const SubtitleStateInfo &state) {
        if (!state.isValid())
            return;
        for (auto &f : state.mpv())
            engine.addSubtitleStream(f.path, f.encoding);
        auto loaded = state.load();
        subtitle.setComponents(loaded);
        engine.setCurrentSubtitleStream(state.track());
        syncSubtitleFileMenu();
    }

    void initWidget() {
        view = new MainView(p);
        UtilObject::setQmlEngine(view->engine());
        auto format = view->requestedFormat();
        if (OpenGLCompat::hasExtension(OpenGLCompat::Debug))
            format.setOption(QSurfaceFormat::DebugContext);
        view->setFormat(format);
        view->setPersistentOpenGLContext(true);
        view->setPersistentSceneGraph(true);

        auto widget = createWindowContainer(view, p);
        auto l = new QVBoxLayout;
        l->addWidget(widget);
        l->setMargin(0);
        p->setLayout(l);
        p->setFocusProxy(widget);
        p->setFocus();
//        widget->setFocus();

        subtitleView = new SubtitleView(p);
        p->setAcceptDrops(true);
        p->resize(400, 300);
        p->setMinimumSize(QSize(400, 300));

        connect(view, &QQuickView::sceneGraphInitialized, p, [this] () {
            auto context = view->openglContext();
            OpenGLCompat::initialize(context);
            auto *logger = OpenGLCompat::logger();
            if (logger) {
                connect(logger, &QOpenGLDebugLogger::messageLogged, p
                    , [] (const QOpenGLDebugMessage & msg) { OpenGLCompat::debug(msg); }, Qt::DirectConnection);
#ifdef CMPLAYER_RELEASE
                logger->startLogging(QOpenGLDebugLogger::AsynchronousLogging);
#else
                logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
#endif
            }
            initialize_vdpau_interop(context);
        }, Qt::DirectConnection);
        connect(view, &QQuickView::sceneGraphInvalidated, p, [this] () {
            auto context = QOpenGLContext::currentContext();
            finalize_vdpau_interop(context);
            OpenGLCompat::finalize(context);
        }, Qt::DirectConnection);
        desktop = cApp.desktop();
        auto reset = [this] () {
            if (!desktop->isVirtualDesktop())
                virtualDesktopSize = QSize();
            else {
                const int count = desktop->screenCount();
                QRect rect = desktop->availableGeometry(0);
                for (int i=1; i<count; ++i)
                    rect |= desktop->availableGeometry(i);
                virtualDesktopSize = rect.size();
            }
        };
        connect(desktop, &QDesktopWidget::resized, reset);
        connect(desktop, &QDesktopWidget::screenCountChanged, reset);
        connect(desktop, &QDesktopWidget::workAreaResized, reset);
        reset();
    }

    int resume(const Mrl &mrl, int *edition) {
        if (!pref().remember_stopped || mrl.isImage() || !mrl.isUnique())
            return 0;
        auto state = history.find(mrl.toUnique());
        if (!state || state->resume_position <= 0)
            return 0;
        *edition = state->edition;
        if (!preferences.ask_record_found)
            return state->resume_position;
        MBox mbox(p, MBox::Icon::Question, tr("Resume Playback"));
        mbox.setText(tr("Do you want to resume the playback at the last played position?"));
        QString time = _MSecToString(state->resume_position, "h:mm:ss");
        if (state->edition >= 0)
            time += _L('[') % tr("Title %1").arg(state->edition) % _L(']');
        mbox.setInformativeText(tr("Played Date: %1\nStopped Position: %2").arg(state->last_played_date_time.toString(Qt::ISODate)).arg(time));
        mbox.checkBox()->setText(tr("Don't ask again"));
        mbox.addButtons({BBox::Yes, BBox::No});
        int resume = 0;
        if (mbox.exec() == BBox::Yes)
            resume = state->resume_position;
        else
            *edition = -1;
        if (_Change(preferences.ask_record_found, !mbox.isChecked()))
            preferences.save();
        return resume;
    }
    int cache(const Mrl &mrl) {
        if (mrl.isLocalFile()) {
            auto path = mrl.toLocalFile();
            if (_ContainsIf(pref().network_folders, [path] (const QString &folder) {
                return path.startsWith(folder);
            }))
                return pref().cache_network;
            return pref().cache_local;
        } if (mrl.isDisc())
            return pref().cache_disc;
        return pref().cache_network;
    }
    void initEngine() {
        engine.setVideoRenderer(&renderer);
        connect(&engine, &PlayEngine::mrlChanged, p, &MainWindow::updateMrl);
        connect(&engine, &PlayEngine::stateChanged, p, [this] (PlayEngine::State state) {
            stateChanging = true;
            showMessageBox(QString());
            if ((loading = state & (PlayEngine::Loading | PlayEngine::Buffering)))
                loadingTimer.start();
            else
                loadingTimer.stop();
            if (state == PlayEngine::Error)
                showMessageBox(tr("Error!\nCannot open the media."));
            switch (state) {
            case PlayEngine::Loading:
            case PlayEngine::Buffering:
            case PlayEngine::Playing:
                menu("play")["pause"]->setText(tr("Pause"));
                break;
            default:
                menu("play")["pause"]->setText(tr("Play"));
            }
            cApp.setScreensaverDisabled(pref().disable_screensaver && state == PlayEngine::Playing);
            p->updateStaysOnTop();
            stateChanging = false;
        });
        connect(&engine, &PlayEngine::tick, &subtitle, &SubtitleRendererItem::render);
        connect(&engine, &PlayEngine::volumeChanged, p, [this] (int volume) { _Change(as.state.audio_volume, volume); });
        connect(&engine, &PlayEngine::volumeNormalizerActivatedChanged, menu("audio")["normalizer"], &QAction::setChecked);
        connect(&engine, &PlayEngine::tempoScaledChanged, menu("audio")["tempo-scaler"], &QAction::setChecked);
        connect(&engine, &PlayEngine::mutedChanged, menu("audio")("volume")["mute"], &QAction::setChecked);
        connect(&engine, &PlayEngine::fpsChanged, &subtitle, &SubtitleRendererItem::setFPS);
        connect(&engine, &PlayEngine::editionsChanged, p, [this] (const EditionList &editions) {
            updateListMenu(menu("play")("title"), editions, engine.currentEdition());
        });
        connect(&engine, &PlayEngine::audioStreamsChanged, p, [this] (const StreamList &streams) {
            updateListMenu(menu("audio")("track"), streams, engine.currentAudioStream());
        });
        connect(&engine, &PlayEngine::videoStreamsChanged, p, [this] (const StreamList &streams) {
            updateListMenu(menu("video")("track"), streams, engine.currentVideoStream());
        });
        connect(&engine, &PlayEngine::subtitleStreamsChanged, p, [this] (const StreamList &streams) {
            updateListMenu(menu("subtitle")("track"), streams, engine.currentSubtitleStream(), _L("internal"));
        });
        connect(&engine, &PlayEngine::chaptersChanged, p, [this] (const ChapterList &chapters) {
            updateListMenu(menu("play")("chapter"), chapters, engine.currentChapter());
        });
        connect(&engine, &PlayEngine::currentAudioStreamChanged, p, [this] (int stream) {
            auto action = menu("audio")("track").g()->find(stream);
            if (action && !action->isChecked()) {
                action->setChecked(true);
                menu("audio")("track").syncActions();
            }
        });
        connect(&engine, &PlayEngine::currentVideoStreamChanged, p, [this] (int stream) {
            auto action = menu("video")("track").g()->find(stream);
            if (action && !action->isChecked()) {
                action->setChecked(true);
                menu("video")("track").syncActions();
            }
        });
        connect(&engine, &PlayEngine::currentSubtitleStreamChanged, p, [this] (int stream) {
            auto actions = menu("subtitle")("track").g("internal")->actions();
            for (auto action : actions)
                action->setChecked(action->data().toInt() == stream);
            menu("subtitle")("track").syncActions();
            setCurrentSubtitleIndexToEngine();
        });
        auto updateMrlState = [this] (const Mrl &mrl, bool end, int time) {
            as.state.mrl = mrl.toUnique();
            as.state.device = mrl.device();
            as.state.last_played_date_time = QDateTime::currentDateTime();
            if (end) {
                as.state.resume_position = time;
                as.state.edition = engine.currentEdition();
                as.state.audio_track = engine.currentAudioStream();
                as.state.sub_track = subtitleState();
                syncState();
            }
            history.update(&as.state, !end);
            as.state.mrl = mrl;
        };

        connect(&engine, &PlayEngine::started, p, [this, updateMrlState] (Mrl mrl) {
            as.setOpen(mrl);
            as.state.mrl = mrl.toUnique();
            auto &state = as.state;
            state.sub_track = SubtitleStateInfo();
            const bool found = history.getState(&state);
            as.state.mrl = mrl;
            if (found) {
                engine.setCurrentAudioStream(state.audio_track);
                syncWithState();
            }
            if (found && state.sub_track.isValid())
                setSubtitleState(state.sub_track);
            else
                updateSubtitleState();
            updateMrlState(mrl, false, 0);
        });
        connect(&engine, &PlayEngine::finished, p, [this, updateMrlState] (Mrl mrl, int time, int remain) {
            updateMrlState(mrl, true, remain > 500 ? time : -1);
        });
        connect(&engine, &PlayEngine::videoFormatChanged, p, [this] (const VideoFormat &format) {
            if (pref().fit_to_video && !format.displaySize().isEmpty())
                setVideoSize(format.displaySize());
        });
        connect(&engine, &PlayEngine::requestNextStartInfo, p, [this] () {
            const auto mrl = playlist.nextMrl(); if (!mrl.isEmpty()) load(mrl);
        });
    }
    void initItems() {
        connect(&recent, &RecentInfo::openListChanged, p, &MainWindow::updateRecentActions);
        connect(&hider, &QTimer::timeout, p, [this] () { setCursorVisible(false); });
        connect(&history, &HistoryModel::playRequested, p, [this] (const Mrl &mrl) { p->openMrl(mrl); });
        connect(&playlist, &PlaylistModel::playRequested, p, [this] (int row) { p->openMrl(playlist[row]); });
        connect(&playlist, &PlaylistModel::finished, p, [this] () {
            if (menu("tool")["auto-exit"]->isChecked()) p->exit();
            if (menu("tool")["auto-shutdown"]->isChecked()) cApp.shutdown();
        });
        connect(&subtitle, &SubtitleRendererItem::modelsChanged, subtitleView, &SubtitleView::setModels);

        renderer.setOverlay(&subtitle);
        auto showSize = [this] {
            p->showMessage(_N(qRound(renderer.width())) % _U("\303\227") % _N(qRound(renderer.height())), &pref().show_osd_on_resized);
        };
        connect(&renderer, &VideoRendererItem::widthChanged, showSize);
        connect(&renderer, &VideoRendererItem::heightChanged, showSize);
    }
    void initTimers() {
        hider.setSingleShot(true);

        loadingTimer.setInterval(500);
        loadingTimer.setSingleShot(true);
        connect(&loadingTimer, &QTimer::timeout, p, [this] () {
            if (loading) showMessageBox(tr("Loading ...\nPlease wait for a while."));
        });

        initializer.setSingleShot(true);
        connect(&initializer, &QTimer::timeout, p, [this] () { p->applyPref(); cApp.runCommands(); });
        initializer.start(1);
    }

    template<typename T, typename F, typename GetNew>
    void connectPropertyDiff(ActionGroup *g, const char *asprop, GetNew getNew, void(MrlState::*sig)(), F f) {
        Q_ASSERT(as.state.property(asprop).isValid());
        connect(g, &ActionGroup::triggered, p, [this, getNew, asprop] (QAction *a) {
            const auto old = as.state.property(asprop).value<T>();
            const auto new_ = getNew(a, old);
            if (old != new_) {
                push(new_, old, [this, asprop] (const T &t) {
                    as.state.setProperty(asprop, QVariant::fromValue<T>(t));
                });
            }
        });
        connect(&as.state, sig, f);
        emit (as.state.*sig)();
    }
    template<typename T, typename F>
    void connectPropertyDiff(ActionGroup *g, const char *asprop, T min, T max, void(MrlState::*sig)(), F f) {
        connectPropertyDiff<T, F>(g, asprop, [min, max] (QAction *a, const T &old) { return qBound<T>(min, a->data().value<T>() + old, max); }, sig, f);
    }
    template<typename F>
    void connectPropertyCheckable(QAction *action, const char *asprop, void(MrlState::*sig)(), F f) {
        Q_ASSERT(as.state.property(asprop).isValid() && as.state.property(asprop).type() == QVariant::Bool);
        Q_ASSERT(action->isCheckable());
        connect(action, &QAction::triggered, p, [action, this, asprop] (bool new_) {
            const bool old = as.state.property(asprop).toBool();
            if (new_ != old) {
                push(new_, old, [action, asprop, this](bool checked) {
                    as.state.setProperty(asprop, checked);
                    action->setChecked(checked);
                    p->showMessage(action->text(), checked);
                });
            }
        });
        connect(&as.state, sig, f);
        emit (as.state.*sig)();
    }

    void setVideoSize(const QSize &video) {
        if (p->isFullScreen() || p->isMaximized())
            return;
        // patched by Handrake
        const QSizeF screen = screenSize();
        const QSize size = (p->size() - renderer.size().toSize() + video);
        if (size != p->size()) {
            p->resize(size);
            int dx = 0;
            const int rightDiff = screen.width() - (p->x() + p->width());
            if (rightDiff < 10) {
                if (rightDiff < 0)
                    dx = screen.width() - p->x() - size.width();
                else
                    dx = p->width() - size.width();
            }
            if (dx) {
                int x = p->x() + dx;
                if (x < 0)
                    x = 0;
                p->move(x, p->y());
            }
        }
    }
        HistoryModel history;
};

#ifdef Q_OS_MAC
void qt_mac_set_dock_menu(QMenu *menu);
#endif

MainWindow::MainWindow(QWidget *parent): QWidget(parent, Qt::Window), d(new Data(this)) {
    AppObject::setEngine(&d->engine);
    AppObject::setHistory(&d->history);
    AppObject::setPlaylist(&d->playlist);
    AppObject::setTopLevelItem(&d->topLevelItem);
    AppObject::setDownloader(&d->downloader);
    d->playlist.setDownloader(&d->downloader);

    d->engine.run();
    d->initWidget();
    d->initContextMenu();
    d->initTimers();
    d->initItems();
    d->initEngine();

    d->dontShowMsg = true;

    connectMenus();

    auto &as = AppState::get();
    d->history.getAppState(&as.state);
    d->syncWithState();

    d->playlist.setVisible(as.playlist_visible);
    d->history.setVisible(as.history_visible);

    if (as.win_size.isValid()) {
        auto screen = d->screenSize();
        move(screen.width()*as.win_pos.x(), screen.height()*as.win_pos.y());
        resize(as.win_size);
    }

    d->renderer.setEffects((VideoRendererItem::Effects)as.state.video_effects);
    for (int i=0; i<16; ++i) {
        if ((as.state.video_effects >> i) & 1)
            d->menu("video")("filter").g()->setChecked(1 << i, true);
    }
    d->menu("tool")["auto-exit"]->setChecked(as.auto_exit);

    d->dontShowMsg = false;

    d->playlist.set(d->recent.lastPlaylist());
    if (!d->recent.lastMrl().isEmpty()) {
        d->load(d->recent.lastMrl(), false);
        as.setOpen(d->recent.lastMrl());
    }
    updateRecentActions(d->recent.openList());

    d->winState = d->prevWinState = windowState();

    connect(&cApp, &App::commitDataRequest, [this] () { d->commitData(); });
    connect(&cApp, &App::saveStateRequest, [this] (QSessionManager &session) {
        session.setRestartHint(QSessionManager::RestartIfRunning);
    });

    d->undo = new QUndoStack(this);
    auto undo = d->menu("tool")["undo"];
    auto redo = d->menu("tool")["redo"];
    connect(d->undo, &QUndoStack::canUndoChanged, undo, &QAction::setEnabled);
    connect(d->undo, &QUndoStack::canRedoChanged, redo, &QAction::setEnabled);
    connect(undo, &QAction::triggered, d->undo, &QUndoStack::undo);
    connect(redo, &QAction::triggered, d->undo, &QUndoStack::redo);
    d->menu("tool")["undo"]->setEnabled(d->undo->canUndo());
    d->menu("tool")["redo"]->setEnabled(d->undo->canRedo());

    if (!OpenGLCompat::hasExtension(OpenGLCompat::TextureFloat)) {
        auto &video = d->menu("video");
        auto key = _L(DitheringInfo::typeKey());
        video(key).g(key)->find(Dithering::Fruit)->setDisabled(true);
        key = _L(InterpolatorTypeInfo::typeKey());
        auto disable = [&] (const char *subkey) {
            for (auto a : video(subkey).g(key)->actions()) {
                if (a->data().toInt() != InterpolatorType::Bilinear)
                    a->setDisabled(true);
            }
        };
        disable("chroma-upscaler");
        disable("interpolator");
    }
    if (TrayIcon::isAvailable()) {
        d->tray = new TrayIcon(cApp.defaultIcon(), this);
        connect(d->tray, &TrayIcon::activated, this, [this] (TrayIcon::ActivationReason reason) {
            if (reason == TrayIcon::Trigger)
                setVisible(!isVisible());
            else if (reason == TrayIcon::Context)
                d->contextMenu.exec(QCursor::pos());
            else if (reason == TrayIcon::Show)
                setVisible(true);
            else if (reason == TrayIcon::Quit)
                exit();
        });
        d->tray->setVisible(d->preferences.enable_system_tray);
    }
}

MainWindow::~MainWindow() {
    cApp.setMprisActivated(false);
    d->view->engine()->clearComponentCache();
    exit();
    delete d->view;
    delete d;
}

void MainWindow::connectMenus() {
    Menu &open = d->menu("open");
    connect(open["file"], &QAction::triggered, this, [this] () {
        auto &as = AppState::get();
        const QString filter = Info::mediaExtFilter();
        const QString dir = QFileInfo(as.open_last_file).absolutePath();
        const QString file = _GetOpenFileName(this, tr("Open File"), dir, filter);
        if (!file.isEmpty())
            openMrl(Mrl(file));
    });
    connect(open["folder"], &QAction::triggered, this, [this] () {
        OpenMediaFolderDialog dlg(this);
        if (dlg.exec()) {
            const auto list = dlg.playlist();
            if (!list.isEmpty()) {
                d->playlist.set(list);
                d->load(list.first());
                d->recent.stack(list.first());
            }
        }
    });
    connect(open["url"], &QAction::triggered, this, [this] () {
        GetUrlDialog dlg(this);
        if (dlg.exec()) {
            if (dlg.isPlaylist())
                d->playlist.open(dlg.url(), dlg.encoding());
            else
                openMrl(dlg.url().toString(), dlg.encoding());
        }
    });
    auto openDisc = [this] (const QString &title, QString &device, bool dvd) {
        OpenDiscDialog dlg(this);
        dlg.setIsoEnabled(dvd);
        dlg.setWindowTitle(title);
        dlg.setDeviceList(cApp.devices());
        if (!device.isEmpty())
            dlg.setDevice(device);
        if (dlg.exec())
            device = dlg.device();
        return dlg.result() && !device.isEmpty();
    };

    connect(open["dvd"], &QAction::triggered, this, [openDisc, this] () {
        if (openDisc(tr("Select DVD device"), d->as.dvd_device, true))
            openMrl(Mrl::fromDisc("dvdnav", d->as.dvd_device, -1, true));
    });
    connect(open["bluray"], &QAction::triggered, this, [openDisc, this] () {
        if (openDisc(tr("Select Blu-ray device"), d->as.bluray_device, false))
            openMrl(Mrl::fromDisc("bdnav", d->as.bluray_device, -1, true));
    });
    connect(open("recent").g(), &ActionGroup::triggered, this, [this] (QAction *a) {openMrl(Mrl(a->data().toString()));});
    connect(open("recent")["clear"], &QAction::triggered, &d->recent, &RecentInfo::clear);

    Menu &play = d->menu("play");
    connect(play["stop"], &QAction::triggered, this, [this] () {d->engine.stop();});
    d->connectStepActions(play("speed"), "play_speed", &MrlState::playSpeedChanged, [this]() {
        d->engine.setSpeed(1e-2*d->as.state.play_speed);
    });
    connect(play["pause"], &QAction::triggered, this, &MainWindow::togglePlayPause);
    connect(play("repeat").g(), &ActionGroup::triggered, this, [this] (QAction *a) {
        const int key = a->data().toInt();
        auto msg = [this] (const QString &ex) {showMessage(tr("A-B Repeat"), ex);};
        if (key == 'r') {
            if (d->engine.isStopped()) return;
            if (!d->ab.hasA()) msg(tr("Set A to %1").arg(_Chopped(_MSecToString(d->ab.setAToCurrentTime(), "h:mm:ss.zzz"), 2)));
            else if (!d->ab.hasB()) {
                const int at = d->ab.setBToCurrentTime();
                if ((at - d->ab.a()) < 100) {d->ab.setB(-1); msg(tr("Range is too short!"));}
                else {d->ab.start(); msg(tr("Set B to %1. Start to repeat!").arg(_Chopped(_MSecToString(at, "h:mm:ss.zzz"), 2)));}
            }
        } else if (key == 'q') {d->ab.stop(); d->ab.setA(-1); d->ab.setB(-1); msg(tr("Quit repeating"));}
        else if (key == 's') {d->ab.setAToSubtitleTime(); d->ab.setBToSubtitleTime(); d->ab.start(); msg(tr("Repeat current subtitle"));}
    });
    connect(play["prev"], &QAction::triggered, &d->playlist, &PlaylistModel::playPrevious);
    connect(play["next"], &QAction::triggered, &d->playlist, &PlaylistModel::playNext);
    connect(play("seek").g("relative"), &ActionGroup::triggered, this, [this] (QAction *a) {
        const int diff = a->data().toInt();
        if (diff && !d->engine.isStopped() && d->engine.isSeekable()) {
            d->engine.relativeSeek(diff);
            showMessage(tr("Seeking"), diff/1000, tr("sec"), true);
            d->showTimeLine();
        }
    });
    connect(play("seek").g("frame"), &ActionGroup::triggered, this, [this] (QAction *a) {
        d->engine.stepFrame(a->data().toInt());
    });
    connect(play["disc-menu"], &QAction::triggered, this, [this] () { d->engine.setCurrentEdition(PlayEngine::DVDMenu); });
    connect(play("seek").g("subtitle"), &ActionGroup::triggered, this, [this] (QAction *a) {
        const int key = a->data().toInt();
        const int time = (key < 0 ? d->subtitle.previous() : (key > 0 ? d->subtitle.next() : d->subtitle.current()));
        if (time >= 0) d->engine.seek(time-100);
    });
    connect(play("title").g(), &ActionGroup::triggered, this, [this] (QAction *a) {
        a->setChecked(true); d->engine.setCurrentEdition(a->data().toInt()); showMessage(tr("Current Title"), a->text());
    });
    connect(play("chapter").g(), &ActionGroup::triggered, this, [this] (QAction *a) {
        a->setChecked(true); d->engine.setCurrentChapter(a->data().toInt()); showMessage(tr("Current Chapter"), a->text());
    });
    auto seekChapter = [this] (int offset) {
        if (!d->engine.chapters().isEmpty()) {
            auto target = d->engine.currentChapter() + offset;
            if (target > -2)
                d->engine.setCurrentChapter(target);
        }
    };
    connect(play("chapter")["prev"], &QAction::triggered, this, [seekChapter] () { seekChapter(-1); });
    connect(play("chapter")["next"], &QAction::triggered, this, [seekChapter] () { seekChapter(+1); });

    Menu &video = d->menu("video");
    connect(video("track").g(), &ActionGroup::triggered, this, [this] (QAction *a) {
        a->setChecked(true); d->engine.setCurrentVideoStream(a->data().toInt()); showMessage(tr("Current Video Track"), a->text());
    });
    d->connectEnumActions<VideoRatio>(video("aspect"), "video_aspect_ratio", &MrlState::videoAspectRatioChanged, [this] () {
        d->renderer.setAspectRatio(VideoRatioInfo::data(d->as.state.video_aspect_ratio));
    });
    d->connectEnumActions<VideoRatio>(video("crop"), "video_crop_ratio", &MrlState::videoCropRatioChanged, [this] () {
        d->renderer.setCropRatio(VideoRatioInfo::data(d->as.state.video_crop_ratio));
    });
    connect(video["snapshot"], &QAction::triggered, this, [this] () {
        static SnapshotDialog *dlg = new SnapshotDialog(this);
        dlg->setVideoRenderer(&d->renderer); dlg->setSubtitleRenderer(&d->subtitle); dlg->take();
        if (!dlg->isVisible()) {dlg->adjustSize(); dlg->show();}
    });
    auto setVideoAlignment = [this] () { d->renderer.setAlignment(VerticalAlignmentInfo::data(d->as.state.video_vertical_alignment) | HorizontalAlignmentInfo::data(d->as.state.video_horizontal_alignment)); };
    d->connectEnumActions<VerticalAlignment>(video("align"), "video_vertical_alignment", &MrlState::videoVerticalAlignmentChanged, setVideoAlignment);
    d->connectEnumActions<HorizontalAlignment>(video("align"), "video_horizontal_alignment", &MrlState::videoHorizontalAlignmentChanged, setVideoAlignment);
    d->connectEnumDataActions<MoveToward>(video("move")
        , "video_offset", [this] (const QPoint &diff) { return diff.isNull() ? diff : d->as.state.video_offset + diff; }
        , &MrlState::videoOffsetChanged, [this] () { d->renderer.setOffset(d->as.state.video_offset); });
    d->connectEnumMenu<DeintMode>(video, "video_deinterlacing", &MrlState::videoDeinterlacingChanged, [this] () {
        d->engine.setDeintMode(d->as.state.video_deinterlacing);
    });
    d->connectEnumActions<InterpolatorType>(video("interpolator"), "video_interpolator", &MrlState::videoInterpolatorChanged, [this] () {
        d->renderer.setInterpolator(d->as.state.video_interpolator);
    });
    d->connectEnumActions<InterpolatorType>(video("chroma-upscaler"), "video_chroma_upscaler", &MrlState::videoChromaUpscalerChanged, [this] () {
        d->renderer.setChromaUpscaler(d->as.state.video_chroma_upscaler);
    });
    d->connectEnumMenu<Dithering>(video, "video_dithering", &MrlState::videoDitheringChanged, [this] () {
        d->renderer.setDithering(d->as.state.video_dithering);
    });
    d->connectEnumMenu<ColorRange>(video, "video_range", &MrlState::videoRangeChanged, [this] () {
        d->renderer.setRange(d->as.state.video_range);
    });

    connect(&video("filter"), &Menu::triggered, this, [this] () {
        VideoRendererItem::Effects effects = 0;
        for (auto act : d->menu("video")("filter").actions()) {
            if (act->isChecked())
                effects |= static_cast<VideoRendererItem::Effect>(act->data().toInt());
        }
        if (d->renderer.effects() != effects)
            d->push(effects, d->renderer.effects(), [this] (VideoRendererItem::Effects effects) {
                d->renderer.setEffects(effects);
                for (auto action : d->menu("video")("filter").actions())
                    action->setChecked(action->data().toInt() & effects);
            });
    });
    d->connectEnumDataActions<AdjustColor>(video("color"), "video_color"
        , [this] (const VideoColor &diff) { return diff.isZero() ? diff : d->as.state.video_color + diff; }, &MrlState::videoColorChanged
        , [this] () {
            showMessage(tr("Adjust Video Color"), d->as.state.video_color.getText(d->as.state.video_color & d->renderer.color()));
            d->renderer.setColor(d->as.state.video_color);
    });

    Menu &audio = d->menu("audio");
    connect(audio("track").g(), &ActionGroup::triggered, this, [this] (QAction *a) {
        a->setChecked(true); d->engine.setCurrentAudioStream(a->data().toInt()); showMessage(tr("Current Audio Track"), a->text());
    });
    d->connectStepActions(audio("volume"), "audio_volume", &MrlState::audioVolumeChanged, [this] () {
        auto v = d->as.state.audio_volume; d->engine.setVolume(v);
    });
    d->connectPropertyCheckable(audio("volume")["mute"], "audio_muted", &MrlState::audioMutedChanged, [this] () {
        d->engine.setMuted(d->as.state.audio_muted);
    });
    d->connectStepActions(audio("sync"), "audio_sync", &MrlState::audioSyncChanged, [this] () {
        auto v = d->as.state.audio_sync; d->engine.setAudioSync(v);
    });
    d->connectStepActions(audio("amp"), "audio_amplifier", &MrlState::audioAmpChanged, [this] () {
        auto v = d->as.state.audio_amplifier; d->engine.setAmp(v*1e-2);
    });
    d->connectEnumMenu<ChannelLayout>(audio, "audio_channel_layout", &MrlState::audioChannelLayoutChanged, [this] () {
        d->engine.setChannelLayout(d->as.state.audio_channel_layout);
    });
    d->connectPropertyCheckable(audio["normalizer"], "audio_volume_normalizer", &MrlState::audioVolumeNormalizerChanged, [this] () {
        d->engine.setVolumeNormalizerActivated(d->as.state.audio_volume_normalizer);
    });
    d->connectPropertyCheckable(audio["tempo-scaler"], "audio_tempo_scaler", &MrlState::audioTempoScalerChanged, [this] () {
        d->engine.setTempoScalerActivated(d->as.state.audio_tempo_scaler);
    });
    auto  selectNext = [this] (const QList<QAction*> &actions) {
        if (!actions.isEmpty()) {
            auto it = actions.begin();
            while (it != actions.end() && !(*it++)->isChecked()) ;
            if (it == actions.end())
                it = actions.begin();
            (*it)->trigger();
        }
    };
    connect(audio("track")["next"], &QAction::triggered, this, [this, selectNext] () {
        selectNext(d->menu("audio")("track").g()->actions());
    });

    Menu &sub = d->menu("subtitle");
    d->subtrackSep = sub("track").addSeparator();
    connect(sub("track")["next"], &QAction::triggered, this, [this] () {
        int checked = -1;
        auto list = d->menu("subtitle")("track").g("external")->actions();
        list += d->menu("subtitle")("track").g("internal")->actions();
        if (list.isEmpty() || (list.size() == 1 && list.first()->isChecked()))
            return;
        d->changingSub = true;
        for (int i=0; i<list.size(); ++i) {
            if (list[i]->isChecked()) {
                checked = i;
                list[i]->setChecked(false);
            }
        }
        d->changingSub = false;
        d->subtitle.deselect(-1);
        if (++checked >= list.size())
            checked = 0;
        list[checked]->trigger();
        if (!d->menu("subtitle")("track").g("internal")->checkedAction())
            d->engine.setCurrentSubtitleStream(-2);
    });
    connect(sub("track")["all"], &QAction::triggered, this, [this] () {
        d->subtitle.select(-1);
        for (auto action : d->menu("subtitle")("track").g("external")->actions())
            action->setChecked(true);
        showMessage(tr("Select All Subtitles"), tr("%1 Subtitle(s)").arg(d->subtitle.componentsCount()));
        d->setCurrentSubtitleIndexToEngine();
    });
    connect(sub("track")["hide"], &QAction::triggered, this, [this] (bool hide) {
        if (hide != d->subtitle.isHidden()) {
            d->push(hide, d->subtitle.isHidden(), [this] (bool hide) {
                d->subtitle.setHidden(hide);
                d->engine.setSubtitleStreamsVisible(!hide);
                if (hide)
                    showMessage(tr("Hide Subtitles"));
                else
                    showMessage(tr("Show Subtitles"));
                d->menu("subtitle")("track")["hide"]->setChecked(hide);
            });
        }
    });
    connect(sub("track")["open"], &QAction::triggered, this, [this] () {
        const auto dir = d->engine.mrl().isLocalFile() ? QFileInfo(d->engine.mrl().toLocalFile()).absolutePath() : _L("");
        QString enc = d->pref().sub_enc;
        const auto files = EncodingFileDialog::getOpenFileNames(this, tr("Open Subtitle"), dir, Info::subtitleExtFilter(), &enc);
        if (!files.isEmpty())
            appendSubFiles(files, true, enc);
    });
    connect(sub("track")["reload"], &QAction::triggered, this, [this] () {
        auto state = d->subtitleState();
        clearSubtitleFiles();
        d->setSubtitleState(state);
    });
    connect(sub("track")["clear"], &QAction::triggered, this, &MainWindow::clearSubtitleFiles);
    connect(sub("track").g("external"), &ActionGroup::triggered, this, [this] (QAction *a) {
        if (!d->changingSub) {
            if (a->isChecked())
                d->subtitle.select(a->data().toInt());
            else
                d->subtitle.deselect(a->data().toInt());
        }
        showMessage(tr("Selected Subtitle"), a->text());
        d->setCurrentSubtitleIndexToEngine();
    });
    connect(sub("track").g("internal"), &ActionGroup::triggered, this, [this] (QAction *a) {
        const bool checked = a->isChecked();
        auto actions = d->menu("subtitle")("track").g("internal")->actions();
        for (auto action : actions)
            action->setChecked(false);
        a->setChecked(checked);
        if (checked) {
            d->engine.setCurrentSubtitleStream(a->data().toInt());
            showMessage(tr("Selected Subtitle"), a->text());
        } else
            d->engine.setCurrentSubtitleStream(-1);
        d->setCurrentSubtitleIndexToEngine();
    });
    connect(&sub("track"), &Menu::actionsSynchronized, this, [this] () { d->setSubtitleTracksToEngine(); d->setCurrentSubtitleIndexToEngine(); });
    d->connectEnumMenu<SubtitleDisplay>(sub, "sub_display", &MrlState::subDisplayChanged, [this] () {
        d->renderer.setOverlayOnLetterbox(d->as.state.sub_display == SubtitleDisplay::OnLetterbox);
    });
    d->connectEnumActions<VerticalAlignment>(sub("align"), "sub_alignment", &MrlState::subAlignmentChanged, [this] () {
        d->subtitle.setTopAligned(d->as.state.sub_alignment == VerticalAlignment::Top);
    });
    d->connectStepActions(sub("position"), "sub_position", &MrlState::subPositionChanged, [this] () {
        d->subtitle.setPos(d->as.state.sub_position*1e-2);
    });
    d->connectStepActions(sub("sync"), "sub_sync", &MrlState::subSyncChanged, [this] () {
        d->subtitle.setDelay(d->as.state.sub_sync);
        d->engine.setSubtitleDelay(d->as.state.sub_sync);
    });

    Menu &tool = d->menu("tool");
    auto toggleTool = [this] (const char *name, bool &visible) {
        visible = !visible;
        if (auto item = d->findItem<QObject>(name))
            item->setProperty("show", visible);
    };
    auto selectedIndex = [this] (const char *name) {
        QObject *item = nullptr;
        return (item = d->findItem<QObject>(name)) ? item->property("selectedIndex").toInt() : -1;
    };
    auto selectIndex = [this] (const char *name, int idx) {
        if (auto item = d->findItem<QObject>(name))
            item->setProperty("selectedIndex", idx);
    };
    auto &playlist = tool("playlist");
    connect(playlist["toggle"], &QAction::triggered, &d->playlist, &PlaylistModel::toggle);
    connect(playlist["open"], &QAction::triggered, this, [this] () {
        QString enc;
        const QString file = EncodingFileDialog::getOpenFileName(this, tr("Open File"), QString(), Info::playlistExtFilter(), &enc);
        if (!file.isEmpty())
            d->playlist.open(file, enc);
    });
    connect(playlist["save"], &QAction::triggered, this, [this] () {
        const Playlist &list = d->playlist.playlist();
        if (!list.isEmpty()) {
            auto file = _GetSaveFileName(this, tr("Save File"), QString(), Info::playlistExtFilter());
            if (!file.isEmpty()) {
                if (QFileInfo(file).suffix().isEmpty())
                    file += ".pls";
                list.save(file);
            }
        }
    });
    connect(playlist["clear"], &QAction::triggered, this, [this] () { d->playlist.clear(); });
    connect(playlist["append-file"], &QAction::triggered, this, [this] () {
        const auto filter = Info::mediaExtFilter();
        auto files = _GetOpenFileNames(this, tr("Open File"), QString(), filter);
        Playlist list;
        for (int i=0; i<files.size(); ++i)
            list << Mrl(files[i]);
        d->playlist.append(list);
    });
    connect(playlist["append-url"], &QAction::triggered, this, [this] () {
        GetUrlDialog dlg(this);
        if (dlg.exec()) {
            const Mrl mrl = dlg.url().toString();
            if (mrl.isPlaylist()) {
                Playlist list;
                list.load(mrl, dlg.encoding());
                d->playlist.append(list);
            } else
                d->playlist.append(mrl);
        }
    });
    connect(playlist["remove"], &QAction::triggered, this, [this, selectedIndex] () {
        d->playlist.erase(selectedIndex("playlist"));
    });
    connect(playlist["move-up"], &QAction::triggered, this, [this, selectedIndex, selectIndex] () {
        const auto idx = selectedIndex("playlist");
        if (d->playlist.swap(idx, idx-1))
            selectIndex("playlist", idx-1);
    });
    connect(playlist["move-down"], &QAction::triggered, this, [this, selectedIndex, selectIndex] () {
        const auto idx = selectedIndex("playlist");
        if (d->playlist.swap(idx, idx+1))
            selectIndex("playlist", idx+1);
    });

    auto &history = tool("history");
    connect(history["toggle"], &QAction::triggered, &d->history, &HistoryModel::toggle);
    connect(history["clear"], &QAction::triggered, this, [this] () { d->history.clear(); });

    connect(tool["playinfo"], &QAction::triggered, this, [toggleTool] () {toggleTool("playinfo", AppState::get().playinfo_visible);});
    connect(tool["subtitle"], &QAction::triggered, this, [this] () {d->subtitleView->setVisible(!d->subtitleView->isVisible());});
    connect(tool["pref"], &QAction::triggered, this, [this] () {
        if (!d->prefDlg) {
            d->prefDlg = new PrefDialog(this);
            connect(d->prefDlg, &PrefDialog::applyRequested, this, [this] {d->prefDlg->get(d->preferences); applyPref();});
            connect(d->prefDlg, &PrefDialog::resetRequested, this, [this] {d->prefDlg->set(d->pref());});
        }
        d->prefDlg->set(d->pref());
        d->prefDlg->show();
    });
    connect(tool["find-subtitle"], &QAction::triggered, this, [this] () {
        if (!d->subFindDlg) {
            d->subFindDlg = new SubtitleFindDialog(this);
            connect(d->subFindDlg, &SubtitleFindDialog::loadRequested, this, [this] (const QString &fileName) {
                appendSubFiles(QStringList() << fileName, true, d->pref().sub_enc);
                showMessage(tr("Downloaded"), QFileInfo(fileName).fileName());
            });
        }
        d->subFindDlg->find(d->engine.mrl());
        d->subFindDlg->show();
    });
    connect(tool["reload-skin"], &QAction::triggered, this, &MainWindow::reloadSkin);
    connect(tool["auto-exit"], &QAction::triggered, this, [this] (bool on) {
        if (on != AppState::get().auto_exit)
            d->push(on, AppState::get().auto_exit, [this] (bool on) {
                AppState::get().auto_exit = on;
                showMessage(on ? tr("Exit CMPlayer when the playlist has finished.") : tr("Auto-exit is canceled."));
                d->menu("tool")["auto-exit"]->setChecked(on);
            });
    });
    connect(tool["auto-shutdown"], &QAction::toggled, this, [this] (bool on) {
        if (on) {
            if (MBox::warn(this, tr("Auto-shutdown")
                    , tr("The system will shut down when the play list has finished.")
                    , {BBox::Ok, BBox::Cancel}) == BBox::Cancel) {
                d->menu("tool")["auto-shutdown"]->setChecked(false);
            } else
                showMessage(tr("The system will shut down when the play list has finished."));
        } else
            showMessage(tr("Auto-shutdown is canceled."));
    });

    Menu &win = d->menu("window");        Menu &help = d->menu("help");
    d->connectEnumMenu<StaysOnTop>(d->as, win, "win_stays_on_top", &AppState::winStaysOnTopChanged, [this] () { updateStaysOnTop(); });
    connect(win.g("size"), &ActionGroup::triggered, this, [this] (QAction *a) {setVideoSize(a->data().toDouble());});
    connect(win["minimize"], &QAction::triggered, this, &MainWindow::showMinimized);
    connect(win["maximize"], &QAction::triggered, this, &MainWindow::showMaximized);
    connect(win["close"], &QAction::triggered, this, [this] () { d->menu.hide(); close(); });

    connect(help["about"], &QAction::triggered, this, [this] () {AboutDialog dlg(this); dlg.exec();});
    connect(d->menu["exit"], &QAction::triggered, this, &MainWindow::exit);

    d->connectCurrentStreamActions(&d->menu("play")("title"), &PlayEngine::currentEdition);
    d->connectCurrentStreamActions(&d->menu("play")("chapter"), &PlayEngine::currentChapter);
}

Playlist MainWindow::generatePlaylist(const Mrl &mrl) const {
    if (!mrl.isLocalFile() || !d->pref().enable_generate_playist)
        return Playlist(mrl);
    const auto mode = d->pref().generate_playlist;
    const QFileInfo file(mrl.toLocalFile());
    const QDir dir = file.dir();
    if (mode == GeneratePlaylist::Folder)
        return Playlist().loadAll(dir);
    const auto filter = Info::mediaNameFilter();
    const auto files = dir.entryInfoList(filter, QDir::Files, QDir::Name);
    const auto fileName = file.fileName();
    Playlist list;
    bool prefix = false, suffix = false;
    auto it = files.cbegin();
    for(; it != files.cend(); ++it) {
        static QRegExp rxs("(\\D*)\\d+(.*)");
        static QRegExp rxt("(\\D*)\\d+(.*)");
        if (rxs.indexIn(fileName) == -1)
            continue;
        if (rxt.indexIn(it->fileName()) == -1)
            continue;
        if (!prefix && !suffix) {
            if (rxs.cap(1) == rxt.cap(1))
                prefix = true;
            else if (rxs.cap(2) == rxt.cap(2))
                suffix = true;
            else
                continue;
        } else if (prefix) {
            if (rxs.cap(1) != rxt.cap(1))
                continue;
        } else if (suffix) {
            if (rxs.cap(2) != rxt.cap(2))
                continue;
        }
        list.append(it->absoluteFilePath());
    }
    if (list.isEmpty())
        return Playlist(mrl);
    return list;
}

void MainWindow::openFromFileManager(const Mrl &mrl) {
    if (mrl.isPlaylist())
        d->playlist.open(mrl);
    else
        d->openWith(d->pref().open_media_from_file_manager, QList<Mrl>() << mrl);
}

PlayEngine *MainWindow::engine() const {
    return &d->engine;
}

PlaylistModel *MainWindow::playlist() const {
    return &d->playlist;
}

void MainWindow::exit() {
    static bool done = false;
    if (!done) {
        cApp.setScreensaverDisabled(false);
        d->commitData();
        d->renderer.setOverlay(nullptr);
        cApp.quit();
        done = true;
    }
}

void MainWindow::play() {
    if (d->stateChanging)
        return;
    if (d->pref().pause_to_play_next_image && d->pref().image_duration == 0 && d->engine.mrl().isImage())
        d->menu("play")["next"]->trigger();
    else {
        const auto state = d->engine.state();
        switch (state) {
        case PlayEngine::Playing:
        case PlayEngine::Loading:
            break;
        case PlayEngine::Paused:
            d->engine.unpause();
            break;
        default:
            d->load(d->engine.mrl());
            break;
        }
    }
}

void MainWindow::togglePlayPause() {
    if (d->stateChanging)
        return;
    if (d->pref().pause_to_play_next_image && d->pref().image_duration == 0 && d->engine.mrl().isImage())
        d->menu("play")["next"]->trigger();
    else {
        const auto state = d->engine.state();
        switch (state) {
        case PlayEngine::Playing:
        case PlayEngine::Loading:
            d->engine.pause();
            break;
        default:
            play();
            break;
        }
    }
}

void MainWindow::updateRecentActions(const QList<Mrl> &list) {
    Menu &recent = d->menu("open")("recent");
    ActionGroup *group = recent.g();
    const int diff = group->actions().size() - list.size();
    if (diff < 0) {
        QList<QAction*> acts = recent.actions();
        QAction *sprt = acts[acts.size()-2];
        Q_ASSERT(sprt->isSeparator());
        for (int i=0; i<-diff; ++i) {
            QAction *action = new QAction(&recent);
            recent.insertAction(sprt, action);
            recent.g()->addAction(action);
        }
    } else if (diff > 0) {
        QList<QAction*> acts = recent.g()->actions();
        for (int i=0; i<diff; ++i)
            delete acts.takeLast();
    }
    QList<QAction*> acts = group->actions();
    for (int i=0; i<list.size(); ++i) {
        QAction *act = acts[i];
        act->setData(list[i].location());
        act->setText(list[i].displayName());
        act->setVisible(!list[i].isEmpty());
    }
    recent.syncActions();
}

void MainWindow::openMrl(const Mrl &mrl) {
    openMrl(mrl, QString());
}

void MainWindow::openMrl(const Mrl &mrl, const QString &enc) {
    if (mrl == d->engine.mrl()) {
        if (!d->engine.startInfo().isValid())
            d->load(mrl);
    } else {
        if (mrl.isPlaylist()) {
            d->playlist.open(mrl, enc);
        } else {
            if (d->playlist.rowOf(mrl) < 0)
                d->playlist.set(generatePlaylist(mrl));
            d->load(mrl);
            if (!mrl.isDvd())
                d->recent.stack(mrl);
        }
    }
}

TopLevelItem *MainWindow::topLevelItem() const {
    return &d->topLevelItem;
}

void MainWindow::showMessage(const QString &message, const bool *force) {
    if (force) {
        if (!*force)
            return;
    } else if (!d->pref().show_osd_on_action)
        return;
    if (!d->dontShowMsg)
        d->showOSD(message);
}

void MainWindow::checkWindowState() {
    const auto state = windowState();
    d->updateWindowSizeState();
    setWindowFilePath(d->filePath);
    if (state != d->winState) {
        d->prevWinState = d->winState;
        d->winState = state;
    }
    d->dontPause = true;
    d->moving = false;
    d->prevPos = QPoint();
    const auto full = isFullScreen();
    if (full) {
        cApp.setAlwaysOnTop(this, false);
        setVisible(true);
    } else {
        updateStaysOnTop();
        setVisible(true);
    }
    d->readyToHideCursor();
    d->dontPause = false;
    if (!d->stateChanging)
        doVisibleAction(state != Qt::WindowMinimized);
    UtilObject::setFullScreen(full);
}

void MainWindow::setFullScreen(bool full) {
    d->dontPause = true;
    if (full != d->fullScreen) {
        d->fullScreen = full;
        d->updateWindowPosState();
#ifdef Q_OS_MAC
        if (!d->pref().lion_style_fullscreen) {
            static Qt::WindowFlags flags = windowFlags();
            static QRect geometry;
            if (full) {
                auto desktop = cApp.desktop();
                const int screen = desktop->screenNumber(this);
                if (screen >= 0) {
                    flags = windowFlags();
                    geometry = this->geometry();
                    setWindowFlags(flags | Qt::FramelessWindowHint);
                    SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
                    show();
                    setGeometry(QRect(QPoint(0, 0), desktop->screenGeometry(this).size()));
                }
            } else {
                setWindowFlags(flags);
                setGeometry(geometry);
                SetSystemUIMode(kUIModeNormal, 0);
            }
            checkWindowState();
            updateTitle();
            updateStaysOnTop();
        } else
#endif
            setWindowState(full ? Qt::WindowFullScreen : (d->prevWinState & ~(Qt::WindowMinimized | Qt::WindowFullScreen)));
    }
    d->dontPause = false;
}

bool MainWindow::isFullScreen() const {return d->fullScreen || QWidget::isFullScreen();}

void MainWindow::setVideoSize(double rate) {
    if (rate < 0) {
        setFullScreen(!isFullScreen());
    } else {
        if (isFullScreen())
            setFullScreen(false);
        if (isMaximized())
            showNormal();
        const QSizeF video = d->renderer.sizeHint();
        if (rate == 0.0)
            rate = _Area(d->screenSize())*0.15/_Area(video);
        d->setVideoSize((video*qSqrt(rate)).toSize());
    }
}

void MainWindow::resetMoving() {
    if (d->moving) {
        d->moving = false;
        d->prevPos = QPoint();
    }
}

void MainWindow::onMouseMoveEvent(QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);
    d->cancelToHideCursor();
    const bool full = isFullScreen();
    const auto gpos = event->globalPos();
    if (full) {
        resetMoving();
    } else {
        if (d->moving) {
            move(this->pos() + gpos - d->prevPos);
            d->prevPos = gpos;
        }
    }
    d->readyToHideCursor();
    d->engine.sendMouseMove(d->renderer.mapToVideo(event->pos()));
}

void MainWindow::onMouseDoubleClickEvent(QMouseEvent *event) {
    QWidget::mouseDoubleClickEvent(event);
    if (event->buttons() & Qt::LeftButton) {
        const auto &info = d->pref().double_click_map[event->modifiers()];
        if (!info.enabled)
            return;
        const auto action = d->menu.action(info.id);
#ifdef Q_OS_MAC
        if (action == d->menu("window")["full"])
            QTimer::singleShot(300, action, SLOT(trigger()));
        else
#endif
        d->trigger(action);
    }
}

void MainWindow::onMouseReleaseEvent(QMouseEvent *event) {
    QWidget::mouseReleaseEvent(event);
    const auto rect = geometry();
    if (d->middleClicked && event->button() == Qt::MiddleButton
            && rect.contains(event->localPos().toPoint()+rect.topLeft())) {
        const auto &info = d->pref().middle_click_map[event->modifiers()];
        if (info.enabled)
            d->trigger(d->menu.action(info.id));
    }
}

void MainWindow::onMousePressEvent(QMouseEvent *event) {
    QWidget::mousePressEvent(event);
    d->middleClicked = false;
    bool showContextMenu = false;
    switch (event->button()) {
    case Qt::LeftButton:
        if (isFullScreen())
            break;
        d->moving = true;
        d->prevPos = event->globalPos();
        break;
    case Qt::MiddleButton:
        d->middleClicked = true;
        break;
    case Qt::RightButton:
        showContextMenu = true;
        break;
    default:
        break;
    }
    if (showContextMenu)
        d->contextMenu.exec(QCursor::pos());
    else
        d->contextMenu.hide();
    d->engine.sendMouseClick(d->renderer.mapToVideo(event->pos()));
}

void MainWindow::onWheelEvent(QWheelEvent *event) {
    QWidget::wheelEvent(event);
    const auto delta = event->delta();
    if (delta) {
        const bool up = d->pref().invert_wheel ? delta < 0 : delta > 0;
        const auto &info = up ? d->pref().scroll_up_map[event->modifiers()]
                              : d->pref().scroll_down_map[event->modifiers()];
        if (info.enabled)
            d->trigger(d->menu.action(info.id));
        event->accept();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
    const auto urls = event->mimeData()->urls();
    if (!event->mimeData()->hasUrls() || urls.isEmpty())
        return;
    Playlist playlist;
    QStringList subList;
    for (int i=0; i<urls.size(); ++i) {
        const QString suffix = QFileInfo(urls[i].path()).suffix().toLower();
        if (Info::playlistExt().contains(suffix)) {
            Playlist list;
            list.load(urls[i]);
            playlist += list;
        } else if (Info::subtitleExt().contains(suffix)) {
            subList << urls[i].toLocalFile();
        } else if (Info::videoExt().contains(suffix)
                || Info::audioExt().contains(suffix)) {
            playlist.append(urls[i]);
        }
    }
    if (!playlist.isEmpty()) {
        d->openWith(d->pref().open_media_by_drag_and_drop, playlist);
    } else if (!subList.isEmpty())
        appendSubFiles(subList, true, d->pref().sub_enc);
}

void MainWindow::reloadSkin() {
    d->player = nullptr;
    d->view->engine()->clearComponentCache();

    d->view->rootContext()->setContextProperty("Util", &UtilObject::get());
    Skin::apply(d->view, d->pref().skin_name);
    if (d->view->status() == QQuickView::Error)
        d->view->setSource(QUrl("qrc:/emptyskin.qml"));
    auto root = d->view->rootObject();
    if (!root)
        return;
    if (root->objectName() == "player")
        d->player = qobject_cast<QQuickItem*>(root);
    if (!d->player)
        d->player = root->findChild<QQuickItem*>("player");
    if (d->player) {
        if (auto item = d->findItem("playinfo"))
            item->setProperty("show", AppState::get().playinfo_visible);
        if (auto item = d->findItem("logo")) {
            item->setProperty("show", d->pref().show_logo);
            item->setProperty("color", d->pref().bg_color);
        }
    }
    d->topLevelItem.setParentItem(d->view->contentItem());
}

void MainWindow::applyPref() {
    int time = -1, title = -1;
    switch (d->engine.state()) {
    case PlayEngine::Playing:
    case PlayEngine::Loading:
    case PlayEngine::Paused:
        time = d->engine.time();
        title = d->engine.currentEdition();
        break;
    default:
        break;
    }
    auto &p = d->pref();
    d->history.setRememberImage(p.remember_image);
    d->history.setPropertiesToRestore(p.restore_properties);
    d->engine.setHwAcc(p.enable_hwaccel ? p.hwaccel_backend : HwAcc::None, p.enable_hwaccel ? p.hwaccel_codecs : QList<int>());
    d->engine.setVolumeNormalizerOption(p.normalizer_length, p.normalizer_target, p.normalizer_silence, p.normalizer_min, p.normalizer_max);
    d->engine.setImageDuration(p.image_duration);
    d->engine.setChannelLayoutMap(p.channel_manipulation);
    d->engine.setSubtitleStyle(p.sub_style);
    auto conv = [&p] (const DeintCaps &caps) {
        DeintOption option;
        option.method = caps.method();
        option.doubler = caps.doubler();
        if (caps.hwdec()) {
            if (!caps.supports(DeintDevice::GPU) && !caps.supports(DeintDevice::OpenGL))
                return DeintOption();
            if (caps.supports(DeintDevice::GPU) && p.hwdeints.contains(caps.method()))
                option.device = DeintDevice::GPU;
            else
                option.device = DeintDevice::OpenGL;
        } else
            option.device = caps.supports(DeintDevice::OpenGL) ? DeintDevice::OpenGL : DeintDevice::CPU;
        return option;
    };
    const auto deint_swdec = conv(p.deint_swdec);
    const auto deint_hwdec = conv(p.deint_hwdec);
    d->engine.setDeintOptions(deint_swdec, deint_hwdec);
    d->engine.setAudioDriver(p.audio_driver);
    d->engine.setClippingMethod(p.clipping_method);
    d->engine.setMinimumCache(p.cache_min_playback, p.cache_min_seeking);
    d->renderer.setKernel(p.blur_kern_c, p.blur_kern_n, p.blur_kern_d, p.sharpen_kern_c, p.sharpen_kern_n, p.sharpen_kern_d);
    SubtitleParser::setMsPerCharactor(p.ms_per_char);
    d->subtitle.setPriority(p.sub_priority);
    d->subtitle.setStyle(p.sub_style);
    d->menu.update(p);
    d->menu.syncTitle();
    d->menu.resetKeyMap();

    reloadSkin();

    if (time >= 0) {
        auto info = d->engine.startInfo();
        info.resume = time;
        info.cache = d->cache(info.mrl);
        if (info.mrl.isDisc())
            info.mrl = info.mrl.titleMrl(title);
        d->engine.load(info);
    }

    if (d->tray)
        d->tray->setVisible(p.enable_system_tray);
    d->preferences.save();
    cApp.setMprisActivated(d->preferences.use_mpris2);
}

template<typename Slot>
void connectCopies(Menu &menu, const Slot &slot) {
    QObject::connect(&menu, &Menu::aboutToShow, slot);
    for (QMenu *copy : menu.copies()) {
        QObject::connect(copy, &QMenu::aboutToShow, slot);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (!d->fullScreen)
        d->updateWindowSizeState();
}

void MainWindow::onKeyPressEvent(QKeyEvent *event) {
    QWidget::keyPressEvent(event);
    constexpr int modMask = Qt::SHIFT | Qt::CTRL | Qt::ALT | Qt::META;
    const QKeySequence shortcut(event->key() + (event->modifiers() & modMask));
    d->trigger(RootMenu::instance().action(shortcut));
    event->accept();
}


void MainWindow::doVisibleAction(bool visible) {
    d->visible = visible;
    if (d->visible) {
        if (d->pausedByHiding && d->engine.isPaused()) {
            d->engine.unpause();
            d->pausedByHiding = false;
        }
        setWindowFilePath(d->filePath);
#ifndef Q_OS_MAC
        setWindowIcon(cApp.defaultIcon());
#endif
    } else {
        if (!d->pref().pause_minimized || d->dontPause)
            return;
        if (!d->engine.isPlaying() || (d->pref().pause_video_only && !d->engine.hasVideo()))
            return;
        d->pausedByHiding = true;
        d->engine.pause();
    }
}

void MainWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    doVisibleAction(true);
}

void MainWindow::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    doVisibleAction(false);
}

void MainWindow::changeEvent(QEvent *event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange)
        checkWindowState();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QWidget::closeEvent(event);
#ifndef Q_OS_MAC
    if (d->tray && d->pref().enable_system_tray && d->pref().hide_rather_close) {
        hide();
        auto &as = AppState::get();
        if (as.ask_system_tray) {
            MBox mbox(this);
            mbox.setIcon(MBox::Icon::Information);
            mbox.setTitle(tr("System Tray Icon"));
            mbox.setText(tr("CMPlayer will be running in "
                "the system tray when the window closed."));
            mbox.setInformativeText(
                tr("You can change this behavior in the preferences. "
                    "If you want to exit CMPlayer, please use 'Exit' menu."));
            mbox.addButton(BBox::Ok);
            mbox.checkBox()->setText(tr("Do not display this message again"));
            mbox.exec();
            as.ask_system_tray = !mbox.checkBox()->isChecked();
        }
        event->ignore();
    } else {
        event->accept();
        exit();
    }
#else
    event->accept();
#endif
}

void MainWindow::updateStaysOnTop() {
    if (windowState() & Qt::WindowMinimized)
        return;
    d->sotChanging = true;
    const auto id = d->as.win_stays_on_top;
    bool onTop = false;
    if (!isFullScreen()) {
        if (id == StaysOnTop::Always)
            onTop = true;
        else if (id == StaysOnTop::None)
            onTop = false;
        else
            onTop = d->engine.isPlaying();
    }
    cApp.setAlwaysOnTop(this, onTop);
    d->sotChanging = false;
}

void MainWindow::updateTitle() {
    const auto mrl = d->engine.mrl();
    setWindowFilePath(QString());
    QString fileName;
    if (!mrl.isEmpty()) {
        if (mrl.isLocalFile()) {
            const QFileInfo file(mrl.toLocalFile());
            d->filePath = file.absoluteFilePath();
            fileName = file.fileName();
            if (isVisible())
                setWindowFilePath(d->filePath);
        } else
            fileName = d->engine.mediaName();
    }
    cApp.setWindowTitle(this, fileName);
}

void MainWindow::updateMrl(const Mrl &mrl) {
    updateTitle();
    const auto disc = mrl.isDisc();
    d->playlist.setLoaded(mrl);
    auto menu = d->menu("play")["disc-menu"];
    menu->setEnabled(disc);
    menu->setVisible(disc);
}

void MainWindow::clearSubtitleFiles() {
    d->subtitle.unload();
    qDeleteAll(d->menu("subtitle")("track").g("external")->actions());
    for (auto action : d->menu("subtitle")("track").g("internal")->actions()) {
        auto id = action->data().toInt();
        if (d->engine.subtitleStreams()[id].isExternal())
            d->engine.removeSubtitleStream(id);
    }
}

bool MainWindow::load(Subtitle &sub, const QString &fileName, const QString &encoding) {
    if (sub.load(fileName, encoding, d->pref().sub_enc_autodetection ? d->pref().sub_enc_accuracy*0.01 : -1.0))
        return true;
    d->engine.addSubtitleStream(fileName, encoding);
    return false;
}

void MainWindow::appendSubFiles(const QStringList &files, bool checked, const QString &enc) {
    if (!files.isEmpty()) {
        Subtitle sub;
        for (auto file : files) {
            if (load(sub, file, enc))
                d->subtitle.load(sub, checked);
        }
        d->syncSubtitleFileMenu();
    }
}

void MainWindow::moveEvent(QMoveEvent *event) {
    QWidget::moveEvent(event);
    if (!d->fullScreen)
        d->updateWindowPosState();
}
