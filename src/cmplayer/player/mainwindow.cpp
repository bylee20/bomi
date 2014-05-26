#include "mainwindow.hpp"
#include "app.hpp"
#include "pref.hpp"
#include "appstate.hpp"
#include "rootmenu.hpp"
#include "recentinfo.hpp"
#include "abrepeater.hpp"
#include "playengine.hpp"
#include "historymodel.hpp"
#include "playlistmodel.hpp"
#include "mainquickview.hpp"
#include "misc/trayicon.hpp"
#include "misc/downloader.hpp"
#include "misc/stepactionpair.hpp"
#include "enum/movetoward.hpp"
#include "dialog/mbox.hpp"
#include "dialog/urldialog.hpp"
#include "dialog/prefdialog.hpp"
#include "dialog/aboutdialog.hpp"
#include "dialog/opendiscdialog.hpp"
#include "dialog/snapshotdialog.hpp"
#include "dialog/subtitlefinddialog.hpp"
#include "dialog/encodingfiledialog.hpp"
#include "dialog/openmediafolderdialog.hpp"
#include "quick/appobject.hpp"
#include "quick/themeobject.hpp"
#include "opengl/opengllogger.hpp"
#include "video/kernel3x3.hpp"
#include "video/deintoption.hpp"
#include "video/videoformat.hpp"
#include "video/videorendereritem.hpp"
#include "audio/audionormalizeroption.hpp"
#include "subtitle/subtitleview.hpp"
#include "subtitle/subtitlemodel.hpp"
#include "subtitle/subtitle_parser.hpp"
#include "subtitle/subtitlerendereritem.hpp"
#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

//DECLARE_LOG_CONTEXT(Main)

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

struct MainWindow::Data {
    struct EnumGroup {
        EnumGroup() {}
        EnumGroup(const char *p, ActionGroup *g)
            : property(p), group(g) { }
        const char *property = nullptr;
        ActionGroup *group = nullptr;
    };

    Data(MainWindow *p): p(p) {preferences.load();}
    QList<EnumGroup> enumGroups;
    MainQuickView *view = nullptr;
    MainWindow *p = nullptr;
    bool visible = false, sotChanging = false, fullScreen = false;
    QQuickItem *player = nullptr;
    RootMenu menu;
    RecentInfo recent;
    AppState as;
    PlayEngine engine;
    VideoRendererItem renderer;
    SubtitleRendererItem subtitle;
    QPoint prevPos;
    Qt::WindowStates winState = Qt::WindowNoState;
    Qt::WindowStates prevWinState = Qt::WindowNoState;
    bool middleClicked = false, moving = false, changingSub = false;
    bool pausedByHiding = false, dontShowMsg = true, dontPause = false;
    bool stateChanging = false, loading = false, sgInit = false;
    QTimer loadingTimer, hider, initializer;
    ABRepeater ab = {&engine, &subtitle};
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
    const Pref &pref() const {return preferences;}
    QAction *subtrackSep = nullptr;
    QDesktopWidget *desktop = nullptr;
    QSize virtualDesktopSize;
    ThemeObject theme;
    auto syncState() -> void
    {
        for (auto &eg : enumGroups) {
            Q_ASSERT(as.state.property(eg.property).isValid());
            auto data = eg.group->data();
            if (data.isValid())
                as.state.setProperty(eg.property, data);
        }
    }
    auto syncWithState() -> void
    {
        for (auto &eg : enumGroups)
            eg.group->setChecked(as.state.property(eg.property), true);
    }

    auto load(const Mrl &mrl, bool play = true) -> void
    {
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
    auto trigger(QAction *action) -> void
    {
        if (!action)
            return;
        if (view->topLevelItem()->isVisible()) {
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
    auto updateSubtitleState() -> void
    {
        const auto &mrl = as.state.mrl;
        if (!mrl.isLocalFile()) {
            p->clearSubtitleFiles();
            syncSubtitleFileMenu();
            return;
        }
        const auto &pref = preferences;
        const QFileInfo file(mrl.toLocalFile());
        auto autoselection = [&] (const QVector<SubComp> &loaded) {
            QVector<int> selected;
            if (loaded.isEmpty() || !pref.sub_enable_autoselect)
                return selected;
            QSet<QString> langSet;
            const QString base = file.completeBaseName();
            for (int i=0; i<loaded.size(); ++i) {
                bool select = false;
                switch (pref.sub_autoselect) {
                case SubtitleAutoselect::Matched: {
                    const QFileInfo info(loaded[i].fileName());
                    select = info.completeBaseName() == base;
                    break;
                } case SubtitleAutoselect::EachLanguage: {
//            const QString lang = loaded[i].m_comp.language().id();
                    const auto lang = loaded[i].language();
                    if ((select = (!langSet.contains(lang))))
                        langSet.insert(lang);
                    break;
                }  case SubtitleAutoselect::All:
                    select = true;
                    break;
                default:
                    break;
                }
                if (select)
                    selected.append(i);
            }
            if (pref.sub_autoselect == SubtitleAutoselect::Matched
                    && !selected.isEmpty() && !pref.sub_ext.isEmpty()) {
                for (int i=0; i<selected.size(); ++i) {
                    const auto fileName = loaded[selected[i]].fileName();
                    const auto suffix = QFileInfo(fileName).suffix();
                    if (pref.sub_ext == suffix.toLower()) {
                        const int idx = selected[i];
                        selected.clear();
                        selected.append(idx);
                        break;
                    }
                }
            }
            return selected;
        };
        auto autoload = [&](bool autoselect) {
            QVector<SubComp> loaded;
            if (!pref.sub_enable_autoload)
                return loaded;
            const QFileInfo fileInfo(mrl.toLocalFile());
            static const auto filter = _ToNameFilter(SubtitleExt);
            const auto dir = fileInfo.dir();
            const auto all = dir.entryInfoList(filter, QDir::Files, QDir::Name);
            const auto base = fileInfo.completeBaseName();
            for (int i=0; i<all.size(); ++i) {
                if (pref.sub_autoload != SubtitleAutoload::Folder) {
                    if (pref.sub_autoload == SubtitleAutoload::Matched) {
                        if (base != all[i].completeBaseName())
                            continue;
                    } else if (!all[i].fileName().contains(base))
                        continue;
                }
                Subtitle sub;
                if (p->load(sub, all[i].absoluteFilePath(), pref.sub_enc)) {
                    for (int i=0; i<sub.size(); ++i)
                        loaded.push_back(sub[i]);
                }
            }
            if (autoselect) {
                const QVector<int> selected = autoselection(loaded);
                for (int i=0; i<selected.size(); ++i)
                    loaded[selected[i]].selection() = true;
            }
            return loaded;
        };
        subtitle.setComponents(autoload(true));
        syncSubtitleFileMenu();
    }

    auto cancelToHideCursor() -> void
    {
        hider.stop();
        view->setCursorVisible(true);
    }

    auto readyToHideCursor() -> void
    {
        if (pref().hide_cursor
                && (p->isFullScreen() || !pref().hide_cursor_fs_only))
            hider.start(pref().hide_cursor_delay);
        else
            cancelToHideCursor();
    }

    auto initContextMenu() -> void
    {
        auto d = this;
        auto addContextMenu = [d] (Menu &menu)
            { d->contextMenu.addMenu(menu.copied(&d->contextMenu)); };
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
        auto addMenuBar = [this, mb] (Menu &menu)
            {mb->addMenu(menu.copied(mb));};
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

    auto openWith(const OpenMediaInfo &mode, const QList<Mrl> &mrls) -> void
    {
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
            switch (mode.behavior) {
            case OpenMediaBehavior::Append:
                d->playlist.append(mrls);
                break;
            case OpenMediaBehavior::ClearAndAppend:
                d->playlist.clear();
                playlist.append(mrls);
                break;
            case OpenMediaBehavior::NewPlaylist:
                d->playlist.clear();
                playlist = p->generatePlaylist(mrl);
                break;
            }
            d->playlist.merge(playlist);
            d->load(mrl, mode.start_playback);
            if (!mrl.isDvd())
                d->recent.stack(mrl);
        }
        if (!p->isVisible())
            p->show();
    }

    auto lastCheckedSubtitleIndex() const -> int
    {
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
    auto setCurrentSubtitleIndexToEngine() -> void
        { engine.setCurrentSubtitleIndex(lastCheckedSubtitleIndex()); }
    auto setSubtitleTracksToEngine() -> void
    {
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

    auto syncSubtitleFileMenu() -> void
    {
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

    auto updateWindowSizeState() -> void
    {
        if (!p->isFullScreen() && !p->isMinimized() && p->isVisible())
            as.win_size = p->size();
    }
    auto screenSize() const -> QSize
    {
        if (desktop->isVirtualDesktop())
            return virtualDesktopSize;
        return desktop->availableGeometry(p).size();
    }
    auto updateWindowPosState() -> void
    {
        if (!p->isFullScreen() && !p->isMinimized() && p->isVisible()) {
            const auto screen = screenSize();
            as.win_pos.rx() = qBound(0.0, p->x()/(double)screen.width(), 1.0);
            as.win_pos.ry() = qBound(0.0, p->y()/(double)screen.height(), 1.0);
        }
    }
    template<class List>
    auto updateListMenu(Menu &menu, const List &list,
                        int current, const QString &group = "") -> void
    {
        if (group.isEmpty())
            menu.setEnabledSync(!list.isEmpty());
        if (!list.isEmpty()) {
            menu.g(group)->clear();
            for (auto it = list.begin(); it != list.end(); ++it) {
                auto act = menu.addActionToGroupWithoutKey(it->name(),
                                                           true, group);
                act->setData(it->id());
                if (current == it->id())
                    act->setChecked(true);
            }
        } else if (!group.isEmpty()) // partial in menu
            menu.g(group)->clear();
        menu.syncActions();
    }
    template<class F>
    auto connectCurrentStreamActions(Menu *menu, F func,
                                     QString group = "") -> void
    {
        auto checkCurrentStreamAction = [this, func, menu, group] () {
            const auto current = (engine.*func)();
            for (auto action : menu->g(group)->actions()) {
                if (action->data().toInt() == current) {
                    action->setChecked(true);
                    break;
                }
            }
        };
        connect(menu, &QMenu::aboutToShow, p, checkCurrentStreamAction);
        for (auto copy : menu->copies())
            connect(copy, &QMenu::aboutToShow, p, checkCurrentStreamAction);
    }
    auto commitData() -> void
    {
        static bool first = true;
        if (first) {
            recent.setLastPlaylist(playlist.list());
            recent.setLastMrl(engine.mrl());
            engine.shutdown();
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

    template<class T, class Func>
    auto push(const T &to, const T &from, const Func &func) -> QUndoCommand*
    {
        if (undo) {
            auto cmd = new ValueCmd<Func, T>(to, from, func);
            undo->push(cmd);
            return cmd;
        } else {
            func(to);
            return nullptr;
        }
    }
    auto showTimeLine() -> void
    {
        if (player && pref().show_osd_timeline)
            QMetaObject::invokeMethod(player, "showTimeLine");
    }
    auto showMessageBox(const QVariant &msg) -> void
    {
        if (player)
            QMetaObject::invokeMethod(player, "showMessageBox", Q_ARG(QVariant, msg));
    }
    auto showOSD(const QVariant &msg) -> void
    {
        if (player)
            QMetaObject::invokeMethod(player, "showOSD", Q_ARG(QVariant, msg));
    }

    template<class T, class F, class Handler, class State>
    auto connectEnumActions(State &state, Menu &menu, Handler handler,
                            const char *asprop, Signal<State> sig, F f) -> void
    {
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
        connect(group, &QActionGroup::triggered, p, handler);
        connect(&state, sig, p, f);
        group->trigger(state.property(asprop));
        emit (state.*sig)();
        if (tmp::is_same<State, MrlState>())
            enumGroups.append(EnumGroup{asprop, group});
    }
    template<class T, class F, class State>
    auto connectEnumActions(State &state, Menu &menu, const char *asprop,
                            Signal<State> sig, F f) -> void
    {
        auto group = menu.g(_L(EnumInfo<T>::typeKey()));
        connectEnumActions<T, F>(state, menu, [=, &state, &menu] (QAction *a)
        {
            auto action = static_cast<EnumAction<T>*>(a);
            auto t = action->enum_();
            auto current = state.property(asprop).template value<T>();
            if (current != t)
                push<T>(t, current, [=, &state, &menu] (T t)
                {
                    if (auto action = group->find(t)) {
                        action->setChecked(true);
                        p->showMessage(menu.title(), action->text());
                        state.setProperty(asprop, action->data());
                    }
                });
        }, asprop, sig, f);
    }
    template<class T, class F>
    auto connectEnumActions(Menu &menu, const char *asprop,
                            Signal<MrlState> sig, F f) -> void
        { connectEnumActions<T, F, MrlState>(as.state, menu, asprop, sig, f); }

    template<class T, class F, class GetNew, class State>
    auto connectEnumDataActions(State &state, Menu &menu, const char *asprop,
                                GetNew getNew, Signal<State> sig, F f) -> void
    {
        using Data = typename EnumInfo<T>::Data;
        connectEnumActions<T, F>(state, menu, [=, &state, &menu] (QAction *a) {
            auto action = static_cast<EnumAction<T>*>(a);
            auto old = state.property(asprop).template value<Data>();
            auto new_ = getNew(action->data());
            auto step = dynamic_cast<StepAction*>(action);
            if (step)
                new_ = step->clamp(new_);
            if (old != new_)
                push<Data>(new_, old, [=, &state, &menu] (const Data &data) {
                    state.setProperty(asprop, QVariant::fromValue<Data>(data));
                    if (step)
                        p->showMessage(menu.title(), step->format(data));
                });
        }, asprop, sig, f);
    }
    template<class T, class F, class GetNew>
    auto connectEnumDataActions(Menu &menu, const char *asprop, GetNew getNew,
                                Signal<MrlState> sig, F f) -> void
    {
        connectEnumDataActions<T, F, GetNew, MrlState>(as.state, menu, asprop,
                                                       getNew, sig, f);
    }

    template<class T, class F, class State>
    auto connectEnumMenu(State &state, Menu &parent, const char *asprop,
                         Signal<State> sig, F f) -> void
    {
        connectEnumActions<T, F>(state, parent(EnumInfo<T>::typeKey()),
                                 asprop, sig, f);
    }

    template<class T, class F>
    auto connectEnumMenu(Menu &parent, const char *asprop,
                         Signal<MrlState> sig, F f) -> void
    {
        connectEnumActions<T, F>(as.state, parent(EnumInfo<T>::typeKey()),
                                 asprop, sig, f);
    }
    template<class F>
    auto connectStepActions(Menu &menu, const char *asprop,
                            Signal<MrlState> sig, F f) -> void
    {
        connectEnumDataActions<ChangeValue, F>(menu, asprop, [=] (int diff) {
            return diff ? (as.state.property(asprop).toInt() + diff) : 0;
        }, sig, f);
    }

    auto setOpen(const Mrl &mrl) -> void
    {
        if (mrl.isLocalFile())
            _SetLastOpenFolder(mrl.toLocalFile());
    }

    auto subtitleState() const -> SubtitleStateInfo
    {
        SubtitleStateInfo state;
        state.track() = engine.currentSubtitleStream();
        state.mpv() = engine.subtitleFiles();
        const auto parsed = subtitle.components();
        for (auto c : parsed)
            state.append(*c);
        return state;
    }
    auto setSubtitleState(const SubtitleStateInfo &state) -> void
    {
        if (!state.isValid())
            return;
        for (auto &f : state.mpv())
            engine.addSubtitleStream(f.path, f.encoding);
        auto loaded = state.load();
        subtitle.setComponents(loaded);
        engine.setCurrentSubtitleStream(state.track());
        syncSubtitleFileMenu();
    }

    auto initWidget() -> void
    {
        view = new MainQuickView(p);
        auto format = view->requestedFormat();
        if (OpenGLLogger::isAvailable())
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
            if (cApp.isOpenGLDebugLoggerRequested())
                glLogger.initialize(context);
            engine.initializeGL(view);
            sgInit = true;
            emit p->sceneGraphInitialized();
        }, Qt::DirectConnection);
        connect(view, &QQuickView::sceneGraphInvalidated, p, [this] () {
            sgInit = false;
            auto context = QOpenGLContext::currentContext();
            engine.finalizeGL();
            glLogger.finalize(context);
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

    auto resume(const Mrl &mrl, int *edition) -> int
    {
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
    auto cache(const Mrl &mrl) -> int
    {
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
    auto initEngine() -> void
    {
        engine.setVideoRenderer(&renderer);
        connect(&engine, &PlayEngine::mrlChanged, p, &MainWindow::updateMrl);
        connect(&engine, &PlayEngine::stateChanged, p,
                [this] (PlayEngine::State state) {
            stateChanging = true;
            showMessageBox(QString());
            constexpr auto flags = PlayEngine::Loading | PlayEngine::Buffering;
            if ((loading = state & flags))
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
            const auto disable = pref().disable_screensaver
                                 && state == PlayEngine::Playing;
            cApp.setScreensaverDisabled(disable);
            p->updateStaysOnTop();
            stateChanging = false;
        });
        connect(&engine, &PlayEngine::tick,
                &subtitle, &SubtitleRendererItem::render);
        connect(&engine, &PlayEngine::volumeChanged,
                p, [this] (int volume) {
            _Change(as.state.audio_volume, volume);
        });
        connect(&engine, &PlayEngine::volumeNormalizerActivatedChanged,
                menu("audio")["normalizer"], &QAction::setChecked);
        connect(&engine, &PlayEngine::tempoScaledChanged,
                menu("audio")["tempo-scaler"], &QAction::setChecked);
        connect(&engine, &PlayEngine::mutedChanged,
                menu("audio")("volume")["mute"], &QAction::setChecked);
        connect(&engine, &PlayEngine::fpsChanged,
                &subtitle, &SubtitleRendererItem::setFPS);
        connect(&engine, &PlayEngine::editionsChanged,
                p, [this] (const EditionList &editions) {
            const auto edition = engine.currentEdition();
            updateListMenu(menu("play")("title"), editions, edition);
        });
        connect(&engine, &PlayEngine::audioStreamsChanged,
                p, [this] (const StreamList &streams) {
            const auto stream = engine.currentAudioStream();
            updateListMenu(menu("audio")("track"), streams, stream);
        });
        connect(&engine, &PlayEngine::videoStreamsChanged,
                p, [this] (const StreamList &streams) {
            const auto stream = engine.currentVideoStream();
            updateListMenu(menu("video")("track"), streams, stream);
        });
        connect(&engine, &PlayEngine::subtitleStreamsChanged,
                p, [this] (const StreamList &streams) {
            auto &menu = this->menu("subtitle")("track");
            const auto stream = engine.currentSubtitleStream();
            updateListMenu(menu, streams, stream, _L("internal"));
        });
        connect(&engine, &PlayEngine::chaptersChanged,
                p, [this] (const ChapterList &chapters) {
            const auto chapter = engine.currentChapter();
            updateListMenu(menu("play")("chapter"), chapters, chapter);
        });
        connect(&engine, &PlayEngine::currentAudioStreamChanged,
                p, [this] (int stream) {
            auto action = menu("audio")("track").g()->find(stream);
            if (action && !action->isChecked()) {
                action->setChecked(true);
                menu("audio")("track").syncActions();
            }
        });
        connect(&engine, &PlayEngine::currentVideoStreamChanged,
                p, [this] (int stream) {
            auto action = menu("video")("track").g()->find(stream);
            if (action && !action->isChecked()) {
                action->setChecked(true);
                menu("video")("track").syncActions();
            }
        });
        connect(&engine, &PlayEngine::currentSubtitleStreamChanged,
                p, [this] (int stream) {
            auto actions = menu("subtitle")("track").g("internal")->actions();
            for (auto action : actions)
                action->setChecked(action->data().toInt() == stream);
            menu("subtitle")("track").syncActions();
            setCurrentSubtitleIndexToEngine();
        });

        connect(&engine, &PlayEngine::started, p, [this] (Mrl mrl) {
            setOpen(mrl);
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
            as.state.mrl = mrl.toUnique();
            as.state.device = mrl.device();
            as.state.last_played_date_time = QDateTime::currentDateTime();
            history.update(&as.state, true);
            as.state.mrl = mrl;
        });
        connect(&engine, &PlayEngine::finished,
                p, [this] (const FinishInfo &info) {
            as.state.mrl = info.mrl.toUnique();
            as.state.device = info.mrl.device();
            as.state.last_played_date_time = QDateTime::currentDateTime();
            as.state.resume_position = info.remain > 500 ? info.position : -1;
            as.state.edition = engine.currentEdition();
            as.state.audio_track = info.streamIds[Stream::Audio];
            as.state.sub_track = subtitleState();
            as.state.sub_track.track() = info.streamIds[Stream::Subtitle];
            syncState();
            history.update(&as.state, false);
            as.state.mrl = info.mrl;
        });
        connect(&engine, &PlayEngine::videoFormatChanged,
                p, [this] (const VideoFormat &format) {
            if (pref().fit_to_video && !format.displaySize().isEmpty())
                setVideoSize(format.displaySize());
        });
        connect(&engine, &PlayEngine::requestNextStartInfo, p, [this] () {
            const auto mrl = playlist.nextMrl(); if (!mrl.isEmpty()) load(mrl);
        });
    }
    auto initItems() -> void
    {
        connect(&recent, &RecentInfo::openListChanged,
                p, &MainWindow::updateRecentActions);
        connect(&hider, &QTimer::timeout,
                p, [this] () { view->setCursorVisible(false); });
        connect(&history, &HistoryModel::playRequested,
                p, [this] (const Mrl &mrl) { p->openMrl(mrl); });
        connect(&playlist, &PlaylistModel::playRequested,
                p, [this] (int row) { p->openMrl(playlist.at(row)); });
        connect(&playlist, &PlaylistModel::finished, p, [this] () {
            if (menu("tool")["auto-exit"]->isChecked()) p->exit();
            if (menu("tool")["auto-shutdown"]->isChecked()) cApp.shutdown();
        });
        connect(&subtitle, &SubtitleRendererItem::modelsChanged,
                subtitleView, &SubtitleView::setModels);

        renderer.setOverlay(&subtitle);
        auto showSize = [this] {
            const auto num = [] (qreal n) { return _N(qRound(n)); };
            const auto w = num(renderer.width()), h = num(renderer.height());
            p->showMessage(w % _U("\303\227") % h, &pref().show_osd_on_resized);
        };
        connect(&renderer, &VideoRendererItem::sizeChanged, p, showSize);
    }
    auto initTimers() -> void
    {
        hider.setSingleShot(true);

        loadingTimer.setInterval(500);
        loadingTimer.setSingleShot(true);
        connect(&loadingTimer, &QTimer::timeout, p, [this] () {
            if (loading)
                showMessageBox(tr("Loading ...\nPlease wait for a while."));
        });

        initializer.setSingleShot(true);
        connect(&initializer, &QTimer::timeout,
                p, [this] () { p->applyPref(); cApp.runCommands(); });
        initializer.start(1);
    }

    template<class T, class F, class GetNew>
    auto connectPropertyDiff(ActionGroup *g, const char *asprop, GetNew getNew,
                             Signal<MrlState> sig, F f) -> void
    {
        Q_ASSERT(as.state.property(asprop).isValid());
        connect(g, &ActionGroup::triggered, p, [=] (QAction *a) {
            const auto old = as.state.property(asprop).value<T>();
            const auto new_ = getNew(a, old);
            if (old != new_) {
                push(new_, old, [=] (const T &t) {
                    as.state.setProperty(asprop, QVariant::fromValue<T>(t));
                });
            }
        });
        connect(&as.state, sig, f);
        emit (as.state.*sig)();
    }
    template<class T, class F>
    auto connectPropertyDiff(ActionGroup *g, const char *asprop, T min, T max,
                             Signal<MrlState> sig, F f) -> void
    {
        connectPropertyDiff<T, F>(g, asprop, [=] (QAction *a, const T &old) {
            return qBound<T>(min, a->data().value<T>() + old, max);
        }, sig, f);
    }
    template<class F>
    auto connectPropertyCheckable(QAction *action, const char *asprop,
                                  Signal<MrlState> sig, F f) -> void
    {
        Q_ASSERT(as.state.property(asprop).isValid()
                 && as.state.property(asprop).type() == QVariant::Bool);
        Q_ASSERT(action->isCheckable());
        connect(action, &QAction::triggered, p, [=] (bool new_) {
            const bool old = as.state.property(asprop).toBool();
            if (new_ != old) {
                push(new_, old, [=](bool checked) {
                    as.state.setProperty(asprop, checked);
                    action->setChecked(checked);
                    p->showMessage(action->text(), checked);
                });
            }
        });
        connect(&as.state, sig, f);
        emit (as.state.*sig)();
    }

    auto setVideoSize(const QSize &video) -> void
    {
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

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent, Qt::Window)
    , d(new Data(this))
{
    AppObject::setEngine(&d->engine);
    AppObject::setHistory(&d->history);
    AppObject::setPlaylist(&d->playlist);
    AppObject::setDownloader(&d->downloader);
    AppObject::setTheme(&d->theme);
    d->playlist.setDownloader(&d->downloader);

    d->engine.run();
    d->initWidget();
    d->initContextMenu();
    d->initTimers();
    d->initItems();
    d->initEngine();

    d->dontShowMsg = true;

    connectMenus();

    d->history.getAppState(&d->as.state);
    d->syncWithState();

    d->playlist.setVisible(d->as.playlist_visible);
    d->history.setVisible(d->as.history_visible);

    if (d->as.win_size.isValid()) {
        auto screen = d->screenSize();
        const auto x = screen.width() * d->as.win_pos.x();
        const auto y = screen.height() * d->as.win_pos.y();
        move(x, y);
        resize(d->as.win_size);
    }

    d->renderer.setEffects(d->as.state.video_effects);
    auto effectGroup = d->menu("video")("filter").g();
    for (auto &item : VideoEffectInfo::items()) {
        if (d->as.state.video_effects & item.value)
            effectGroup->setChecked(QVariant::fromValue(item.value), true);
    }
    d->menu("tool")["auto-exit"]->setChecked(d->as.auto_exit);

    d->dontShowMsg = false;

    d->playlist.setList(d->recent.lastPlaylist());
    if (!d->recent.lastMrl().isEmpty()) {
        d->load(d->recent.lastMrl(), false);
        d->setOpen(d->recent.lastMrl());
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

    if (!VideoRendererItem::supportsHighQualityRendering()) {
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
        connect(d->tray, &TrayIcon::activated,
                this, [this] (TrayIcon::ActivationReason reason) {
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

auto MainWindow::connectMenus() -> void
{
    Menu &open = d->menu("open");
    connect(open["file"], &QAction::triggered, this, [this] () {
        const auto file = _GetOpenFileName(this, tr("Open File"), MediaExt);
        if (!file.isEmpty())
            openMrl(Mrl(file));
    });
    connect(open["folder"], &QAction::triggered, this, [this] () {
        OpenMediaFolderDialog dlg(this);
        if (dlg.exec()) {
            const auto list = dlg.playlist();
            if (!list.isEmpty()) {
                d->playlist.setList(list);
                d->load(list.first());
                d->recent.stack(list.first());
            }
        }
    });
    connect(open["url"], &QAction::triggered, this, [this] () {
        UrlDialog dlg(this);
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
    connect(open("recent").g(), &ActionGroup::triggered,
            this, [this] (QAction *a) {openMrl(Mrl(a->data().toString()));});
    connect(open("recent")["clear"], &QAction::triggered,
            &d->recent, &RecentInfo::clear);

    Menu &play = d->menu("play");
    connect(play["stop"], &QAction::triggered,
            this, [this] () {d->engine.stop();});
    d->connectStepActions(play("speed"), "play_speed",
                          &MrlState::playSpeedChanged, [this]() {
        d->engine.setSpeed(1e-2*d->as.state.play_speed);
    });
    connect(play["pause"], &QAction::triggered,
            this, &MainWindow::togglePlayPause);
    connect(play("repeat").g(), &ActionGroup::triggered,
            this, [this] (QAction *a) {
        const int key = a->data().toInt();
        auto msg = [this] (const QString &ex)
            { showMessage(tr("A-B Repeat"), ex); };
        auto time = [] (int t)
            { return _Chopped(_MSecToString(t, "h:mm:ss.zzz"), 2); };
        switch (key) {
        case 'r':
            if (d->engine.isStopped())
                break;
            if (!d->ab.hasA()) {
                msg(tr("Set A to %1").arg(time(d->ab.setAToCurrentTime())));
            } else if (!d->ab.hasB()) {
                const int at = d->ab.setBToCurrentTime();
                if ((at - d->ab.a()) < 100) {
                    d->ab.setB(-1);
                    msg(tr("Range is too short!"));
                } else {
                    d->ab.start();
                    msg(tr("Set B to %1. Start to repeat!").arg(time(at)));
                }
            }
            break;
        case 's':
            d->ab.setAToSubtitleTime();
            d->ab.setBToSubtitleTime();
            d->ab.start(); msg(tr("Repeat current subtitle"));
            break;
        case 'q':
            d->ab.stop();
            d->ab.setA(-1);
            d->ab.setB(-1);
            msg(tr("Quit repeating"));
        }
    });
    connect(play["prev"], &QAction::triggered,
            &d->playlist, &PlaylistModel::playPrevious);
    connect(play["next"], &QAction::triggered,
            &d->playlist, &PlaylistModel::playNext);
    connect(play("seek").g("relative"), &ActionGroup::triggered,
            this, [this] (QAction *a) {
        const int diff = static_cast<StepAction*>(a)->data();
        if (diff && !d->engine.isStopped() && d->engine.isSeekable()) {
            d->engine.relativeSeek(diff);
            showMessage(tr("Seeking"), diff/1000, tr("sec"), true);
            d->showTimeLine();
        }
    });
    connect(play("seek").g("frame"), &ActionGroup::triggered,
            this, [this] (QAction *a) {
        d->engine.stepFrame(a->data().toInt());
    });
    connect(play["disc-menu"], &QAction::triggered, this, [this] () {
        d->engine.setCurrentEdition(PlayEngine::DVDMenu);
    });
    connect(play("seek").g("subtitle"), &ActionGroup::triggered,
            this, [this] (QAction *a) {
        const int key = a->data().toInt();
        const int time = key < 0 ? d->subtitle.previous()
                       : key > 0 ? d->subtitle.next()
                                 : d->subtitle.current();
        if (time >= 0)
            d->engine.seek(time-100);
    });
    connect(play("title").g(), &ActionGroup::triggered,
            this, [this] (QAction *a) {
        a->setChecked(true);
        d->engine.setCurrentEdition(a->data().toInt());
        showMessage(tr("Current Title"), a->text());
    });
    connect(play("chapter").g(), &ActionGroup::triggered, this,
            [this] (QAction *a) {
        a->setChecked(true);
        d->engine.setCurrentChapter(a->data().toInt());
        showMessage(tr("Current Chapter"), a->text());
    });
    auto seekChapter = [this] (int offset) {
        if (!d->engine.chapters().isEmpty()) {
            auto target = d->engine.currentChapter() + offset;
            if (target > -2)
                d->engine.setCurrentChapter(target);
        }
    };
    connect(play("chapter")["prev"], &QAction::triggered,
            this, [seekChapter] () { seekChapter(-1); });
    connect(play("chapter")["next"], &QAction::triggered, this,
            [seekChapter] () { seekChapter(+1); });

    Menu &video = d->menu("video");
    connect(video("track").g(), &ActionGroup::triggered,
            this, [this] (QAction *a) {
        a->setChecked(true);
        d->engine.setCurrentVideoStream(a->data().toInt());
        showMessage(tr("Current Video Track"), a->text());
    });
    d->connectEnumActions<VideoRatio>
            (video("aspect"), "video_aspect_ratio",
             &MrlState::videoAspectRatioChanged, [this] () {
        d->renderer.setAspectRatio(_EnumData(d->as.state.video_aspect_ratio));
    });
    d->connectEnumActions<VideoRatio>
            (video("crop"), "video_crop_ratio",
             &MrlState::videoCropRatioChanged, [this] () {
        d->renderer.setCropRatio(_EnumData(d->as.state.video_crop_ratio));
    });
    connect(video["snapshot"], &QAction::triggered, this, [this] () {
        if (!d->snapshot) {
            d->snapshot = new SnapshotDialog(this);
            connect(d->snapshot, &SnapshotDialog::request, this, [=] () {
                if (!d->renderer.hasFrame())
                    d->snapshot->clear();
                else
                    d->renderer.requestFrameImage();
            });
        }
        d->snapshot->take();
    });
    connect(&d->renderer, &VideoRendererItem::frameImageObtained,
            this, [this] (QImage image) {
        Q_ASSERT(d->snapshot);
        QRectF subRect;
        auto sub = d->subtitle.draw(image.rect(), &subRect);
        d->snapshot->setImage(image, sub, subRect);
    }, Qt::QueuedConnection);
    auto setVideoAlignment = [this] () {
        const auto v = _EnumData(d->as.state.video_vertical_alignment);
        const auto h = _EnumData(d->as.state.video_horizontal_alignment);
        d->renderer.setAlignment(v | h);
    };
    d->connectEnumActions<VerticalAlignment>
            (video("align"), "video_vertical_alignment",
             &MrlState::videoVerticalAlignmentChanged, setVideoAlignment);
    d->connectEnumActions<HorizontalAlignment>
            (video("align"), "video_horizontal_alignment",
             &MrlState::videoHorizontalAlignmentChanged, setVideoAlignment);
    d->connectEnumDataActions<MoveToward>
            (video("move"), "video_offset",
             [this] (const QPoint &diff) {
        return diff.isNull() ? diff : d->as.state.video_offset + diff;
    }, &MrlState::videoOffsetChanged, [this] () {
        d->renderer.setOffset(d->as.state.video_offset);
    });
    d->connectEnumMenu<DeintMode>
            (video, "video_deinterlacing",
             &MrlState::videoDeinterlacingChanged, [this] () {
        d->engine.setDeintMode(d->as.state.video_deinterlacing);
    });
    d->connectEnumActions<InterpolatorType>
            (video("interpolator"), "video_interpolator",
             &MrlState::videoInterpolatorChanged, [this] () {
        d->renderer.setInterpolator(d->as.state.video_interpolator);
    });
    d->connectEnumActions<InterpolatorType>
            (video("chroma-upscaler"), "video_chroma_upscaler",
             &MrlState::videoChromaUpscalerChanged, [this] () {
        d->engine.setVideoChromaUpscaler(d->as.state.video_chroma_upscaler);
    });
    d->connectEnumMenu<Dithering>
            (video, "video_dithering",
             &MrlState::videoDitheringChanged, [this] () {
        d->renderer.setDithering(d->as.state.video_dithering);
    });
    d->connectEnumMenu<ColorRange>
            (video, "video_range", &MrlState::videoRangeChanged, [this] () {
        d->engine.setVideoColorRange(d->as.state.video_range);
    });

    connect(&video("filter"), &Menu::triggered, this, [this] () {
        VideoEffects effects = 0;
        for (auto act : d->menu("video")("filter").actions()) {
            if (act->isChecked())
                effects |= act->data().value<VideoEffect>();
        }
        if (d->renderer.effects() != effects)
            d->push(effects, d->renderer.effects(),
                    [this] (VideoEffects effects) {
                d->renderer.setEffects(effects);
                d->as.state.video_effects = effects;
                for (auto a : d->menu("video")("filter").actions())
                    a->setChecked(a->data().value<VideoEffect>() & effects);
            });
    });
    VideoColor::for_type([=, &video] (VideoColor::Type type) {
        const auto name = VideoColor::name(type);
        connect(video("color").g(name), &ActionGroup::triggered, this,
                [=] (QAction *a) {
            const int diff = static_cast<StepAction*>(a)->data();
            auto color = d->as.state.video_color;
            color.add(type, diff);
            const auto text = color.formatText(type).arg(color.get(type));
            showMessage(tr("Adjust Video Color"), text);
            d->as.state.setProperty("video_color", QVariant::fromValue(color));
        });
    });
    connect(video("color")["reset"], &QAction::triggered, this, [this] () {
        showMessage(tr("Reset Video Color"));
        const auto var = QVariant::fromValue(VideoColor());
        d->as.state.setProperty("video_color", var);
    });
    connect(&d->as.state, &MrlState::videoColorChanged,
            &d->engine, &PlayEngine::setVideoEqualizer);

    Menu &audio = d->menu("audio");
    connect(audio("track").g(), &ActionGroup::triggered,
            this, [this] (QAction *a) {
        a->setChecked(true);
        d->engine.setCurrentAudioStream(a->data().toInt());
        showMessage(tr("Current Audio Track"), a->text());
    });
    d->connectStepActions(audio("volume"), "audio_volume",
                          &MrlState::audioVolumeChanged, [this] () {
        d->engine.setVolume(d->as.state.audio_volume);
    });
    d->connectPropertyCheckable(audio("volume")["mute"], "audio_muted",
                                &MrlState::audioMutedChanged, [this] () {
        d->engine.setMuted(d->as.state.audio_muted);
    });
    d->connectStepActions(audio("sync"), "audio_sync",
                          &MrlState::audioSyncChanged, [this] () {
        d->engine.setAudioSync(d->as.state.audio_sync);
    });
    d->connectStepActions(audio("amp"), "audio_amplifier",
                          &MrlState::audioAmpChanged, [this] () {
        d->engine.setAmp(d->as.state.audio_amplifier*1e-2);
    });
    d->connectEnumMenu<ChannelLayout>
            (audio, "audio_channel_layout",
             &MrlState::audioChannelLayoutChanged, [this] () {
        d->engine.setChannelLayout(d->as.state.audio_channel_layout);
    });
    d->connectPropertyCheckable
            (audio["normalizer"], "audio_volume_normalizer",
             &MrlState::audioVolumeNormalizerChanged, [this] () {
        const auto activate = d->as.state.audio_volume_normalizer;
        d->engine.setVolumeNormalizerActivated(activate);
    });
    d->connectPropertyCheckable
            (audio["tempo-scaler"], "audio_tempo_scaler",
             &MrlState::audioTempoScalerChanged, [this] () {
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
    connect(audio("track")["next"], &QAction::triggered,
            this, [this, selectNext] () {
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
        const auto acts = d->menu("subtitle")("track").g("external")->actions();
        for (auto action : acts)
            action->setChecked(true);
        const int count = d->subtitle.componentsCount();
        const auto text = tr("%1 Subtitle(s)").arg(count);
        showMessage(tr("Select All Subtitles"), text);
        d->setCurrentSubtitleIndexToEngine();
    });
    connect(sub("track")["hide"], &QAction::triggered,
            this, [this] (bool hide) {
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
        QString dir;
        if (d->engine.mrl().isLocalFile())
            dir = QFileInfo(d->engine.mrl().toLocalFile()).absolutePath();
        QString enc = d->pref().sub_enc;
        const auto files = EncodingFileDialog::getOpenFileNames
                                    (this, tr("Open Subtitle"), dir,
                                     _ToFilter(SubtitleExt), &enc);
        if (!files.isEmpty())
            appendSubFiles(files, true, enc);
    });
    connect(sub("track")["auto-load"], &QAction::triggered, this, [this] () {
        clearSubtitleFiles();
        d->updateSubtitleState();
    });
    connect(sub("track")["reload"], &QAction::triggered, this, [this] () {
        auto state = d->subtitleState();
        clearSubtitleFiles();
        d->setSubtitleState(state);
    });
    connect(sub("track")["clear"], &QAction::triggered,
            this, &MainWindow::clearSubtitleFiles);
    connect(sub("track").g("external"), &ActionGroup::triggered,
            this, [this] (QAction *a) {
        if (!d->changingSub) {
            if (a->isChecked())
                d->subtitle.select(a->data().toInt());
            else
                d->subtitle.deselect(a->data().toInt());
        }
        showMessage(tr("Selected Subtitle"), a->text());
        d->setCurrentSubtitleIndexToEngine();
    });
    connect(sub("track").g("internal"), &ActionGroup::triggered,
            this, [this] (QAction *a) {
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
    connect(&sub("track"), &Menu::actionsSynchronized, this, [this] () {
        d->setSubtitleTracksToEngine();
        d->setCurrentSubtitleIndexToEngine();
    });
    d->connectEnumMenu<SubtitleDisplay>
            (sub, "sub_display", &MrlState::subDisplayChanged, [this] () {
        const auto on = d->as.state.sub_display == SubtitleDisplay::OnLetterbox;
        d->renderer.setOverlayOnLetterbox(on);
    });
    d->connectEnumActions<VerticalAlignment>
            (sub("align"), "sub_alignment",
             &MrlState::subAlignmentChanged, [this] () {
        const auto top = d->as.state.sub_alignment == VerticalAlignment::Top;
        d->subtitle.setTopAligned(top);
    });
    d->connectStepActions(sub("position"), "sub_position",
                          &MrlState::subPositionChanged, [this] () {
        d->subtitle.setPos(d->as.state.sub_position*1e-2);
    });
    d->connectStepActions(sub("sync"), "sub_sync",
                          &MrlState::subSyncChanged, [this] () {
        d->subtitle.setDelay(d->as.state.sub_sync);
        d->engine.setSubtitleDelay(d->as.state.sub_sync);
    });

    Menu &tool = d->menu("tool");
    auto &playlist = tool("playlist");
    connect(playlist["toggle"], &QAction::triggered,
            &d->playlist, &PlaylistModel::toggle);
    connect(playlist["open"], &QAction::triggered, this, [this] () {
        QString enc;
        const auto filter = _ToFilter(PlaylistExt);
        const auto file = EncodingFileDialog::getOpenFileName
                (this, tr("Open File"), QString(), filter, &enc);
        if (!file.isEmpty())
            d->playlist.open(file, enc);
    });
    connect(playlist["save"], &QAction::triggered, this, [this] () {
        const auto &list = d->playlist.list();
        if (!list.isEmpty()) {
            auto file = _GetSaveFileName(this, tr("Save File"),
                                         QString(), PlaylistExt);
            if (!file.isEmpty())
                list.save(file);
        }
    });
    connect(playlist["clear"], &QAction::triggered,
            this, [this] () { d->playlist.clear(); });
    connect(playlist["append-file"], &QAction::triggered, this, [this] () {
        const auto files = _GetOpenFileNames(this, tr("Open File"), MediaExt);
        Playlist list;
        for (int i=0; i<files.size(); ++i)
            list.push_back(Mrl(files[i]));
        d->playlist.append(list);
    });
    connect(playlist["append-url"], &QAction::triggered, this, [this] () {
        UrlDialog dlg(this);
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
    connect(playlist["remove"], &QAction::triggered, this, [=] () {
        d->playlist.remove(d->playlist.selected());
    });
    connect(playlist["move-up"], &QAction::triggered, this, [=] () {
        const auto idx = d->playlist.selected();
        if (d->playlist.swap(idx, idx-1))
            d->playlist.select(idx-1);
    });
    connect(playlist["move-down"], &QAction::triggered, this, [=] () {
        const auto idx = d->playlist.selected();
        if (d->playlist.swap(idx, idx+1))
            d->playlist.select(idx+1);
    });

    auto &history = tool("history");
    connect(history["toggle"], &QAction::triggered,
            &d->history, &HistoryModel::toggle);
    connect(history["clear"], &QAction::triggered,
            this, [this] () { d->history.clear(); });

    connect(tool["playinfo"], &QAction::triggered, this, [=] () {
        auto toggleTool = [this] (const char *name, bool &visible) {
            visible = !visible;
            if (auto item = d->view->findItem<QObject>(name))
                item->setProperty("show", visible);
        };
        toggleTool("playinfo", d->as.playinfo_visible);
    });
    connect(tool["subtitle"], &QAction::triggered, this, [this] () {
        d->subtitleView->setVisible(!d->subtitleView->isVisible());
    });
    connect(tool["pref"], &QAction::triggered, this, [this] () {
        if (!d->prefDlg) {
            d->prefDlg = new PrefDialog(this);
            connect(d->prefDlg, &PrefDialog::applyRequested, this, [this] {
                d->prefDlg->get(d->preferences); applyPref();
            });
            connect(d->prefDlg, &PrefDialog::resetRequested, this, [this] {
                d->prefDlg->set(d->pref());
            });
        }
        d->prefDlg->set(d->pref());
        d->prefDlg->show();
    });
    connect(tool["find-subtitle"], &QAction::triggered, this, [this] () {
        if (!d->subFindDlg) {
            d->subFindDlg = new SubtitleFindDialog(this);
            connect(d->subFindDlg, &SubtitleFindDialog::loadRequested,
                    this, [this] (const QString &fileName) {
                appendSubFiles(QStringList(fileName), true, d->pref().sub_enc);
                showMessage(tr("Downloaded"), QFileInfo(fileName).fileName());
            });
        }
        d->subFindDlg->find(d->engine.mrl());
        d->subFindDlg->show();
    });
    connect(tool["reload-skin"], &QAction::triggered,
            this, &MainWindow::reloadSkin);
    connect(tool["auto-exit"], &QAction::triggered, this, [this] (bool on) {
        if (on != d->as.auto_exit)
            d->push(on, d->as.auto_exit, [this] (bool on) {
                d->as.auto_exit = on;
                showMessage(on ? tr("Exit CMPlayer when "
                                    "the playlist has finished.")
                               : tr("Auto-exit is canceled."));
                d->menu("tool")["auto-exit"]->setChecked(on);
            });
    });
    connect(tool["auto-shutdown"], &QAction::toggled, this, [this] (bool on) {
        if (on) {
            if (MBox::warn(this, tr("Auto-shutdown"),
                           tr("The system will shut down "
                              "when the play list has finished."),
                           {BBox::Ok, BBox::Cancel}) == BBox::Cancel) {
                d->menu("tool")["auto-shutdown"]->setChecked(false);
            } else
                showMessage(tr("The system will shut down "
                               "when the play list has finished."));
        } else
            showMessage(tr("Auto-shutdown is canceled."));
    });

    Menu &win = d->menu("window");
    d->connectEnumMenu<StaysOnTop>
            (d->as, win, "win_stays_on_top", &AppState::winStaysOnTopChanged,
             [this] () { updateStaysOnTop(); });
    connect(win.g("size"), &ActionGroup::triggered,
            this, [this] (QAction *a) {setVideoSize(a->data().toDouble());});
    connect(win["minimize"], &QAction::triggered,
            this, &MainWindow::showMinimized);
    connect(win["maximize"], &QAction::triggered,
            this, &MainWindow::showMaximized);
    connect(win["close"], &QAction::triggered,
            this, [this] () { d->menu.hide(); close(); });

    Menu &help = d->menu("help");
    connect(help["about"], &QAction::triggered,
            this, [this] () {AboutDialog dlg(this); dlg.exec();});
    connect(d->menu["exit"], &QAction::triggered, this, &MainWindow::exit);

    d->connectCurrentStreamActions(&d->menu("play")("title"),
                                   &PlayEngine::currentEdition);
    d->connectCurrentStreamActions(&d->menu("play")("chapter"),
                                   &PlayEngine::currentChapter);
}

auto MainWindow::generatePlaylist(const Mrl &mrl) const -> Playlist
{
    if (!mrl.isLocalFile() || !d->pref().enable_generate_playist)
        return Playlist(mrl);
    const auto mode = d->pref().generate_playlist;
    const QFileInfo file(mrl.toLocalFile());
    const QDir dir = file.dir();
    if (mode == GeneratePlaylist::Folder)
        return Playlist().loadAll(dir);
    const auto filter = _ToNameFilter(MediaExt);
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

auto MainWindow::openFromFileManager(const Mrl &mrl) -> void
{
    if (mrl.isPlaylist())
        d->playlist.open(mrl);
    else {
        const auto mode = d->pref().open_media_from_file_manager;
        d->openWith(mode, QList<Mrl>() << mrl);
    }
}

auto MainWindow::engine() const -> PlayEngine*
{
    return &d->engine;
}

auto MainWindow::playlist() const -> PlaylistModel*
{
    return &d->playlist;
}

auto MainWindow::exit() -> void
{
    static bool done = false;
    if (!done) {
        cApp.setScreensaverDisabled(false);
        d->commitData();
        d->renderer.setOverlay(nullptr);
        cApp.quit();
        done = true;
    }
}

auto MainWindow::play() -> void
{
    if (d->stateChanging)
        return;
    if (d->pref().pause_to_play_next_image
            && d->pref().image_duration == 0 && d->engine.mrl().isImage())
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

auto MainWindow::togglePlayPause() -> void
{
    if (d->stateChanging)
        return;
    if (d->pref().pause_to_play_next_image
            && d->pref().image_duration == 0 && d->engine.mrl().isImage())
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

auto MainWindow::updateRecentActions(const QList<Mrl> &list) -> void
{
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

auto MainWindow::openMrl(const Mrl &mrl) -> void
{
    openMrl(mrl, QString());
}

auto MainWindow::openMrl(const Mrl &mrl, const QString &enc) -> void
{
    if (mrl == d->engine.mrl()) {
        if (!d->engine.startInfo().isValid())
            d->load(mrl);
    } else {
        if (mrl.isPlaylist()) {
            d->playlist.open(mrl, enc);
        } else {
            if (d->playlist.rowOf(mrl) < 0)
                d->playlist.setList(generatePlaylist(mrl));
            d->load(mrl);
            if (!mrl.isDvd())
                d->recent.stack(mrl);
        }
    }
}

auto MainWindow::isSceneGraphInitialized() const -> bool
{
    return d->sgInit;
}

auto MainWindow::showMessage(const QString &message, const bool *force) -> void
{
    if (force) {
        if (!*force)
            return;
    } else if (!d->pref().show_osd_on_action)
        return;
    if (!d->dontShowMsg)
        d->showOSD(message);
}

auto MainWindow::checkWindowState() -> void
{

}

auto MainWindow::setFullScreen(bool full) -> void
{
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
                    setGeometry(QRect(QPoint(0, 0),
                                      desktop->screenGeometry(this).size()));
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
        {
            constexpr auto minfull = Qt::WindowMinimized | Qt::WindowFullScreen;
            setWindowState(full ? Qt::WindowFullScreen
                                : (d->prevWinState & ~minfull));
        }
        emit fullscreenChanged(d->fullScreen);
    }
    d->dontPause = false;
}

auto MainWindow::isFullScreen() const -> bool
{
    return d->fullScreen || QWidget::isFullScreen();
}

auto MainWindow::setVideoSize(double rate) -> void
{
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

auto MainWindow::resetMoving() -> void
{
    if (d->moving) {
        d->moving = false;
        d->prevPos = QPoint();
    }
}

auto MainWindow::onMouseMoveEvent(QMouseEvent *event) -> void
{
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

auto MainWindow::onMouseDoubleClickEvent(QMouseEvent *event) -> void
{
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

auto MainWindow::onMouseReleaseEvent(QMouseEvent *event) -> void
{
    QWidget::mouseReleaseEvent(event);
    const auto rect = geometry();
    if (d->middleClicked && event->button() == Qt::MiddleButton
            && rect.contains(event->localPos().toPoint()+rect.topLeft())) {
        const auto &info = d->pref().middle_click_map[event->modifiers()];
        if (info.enabled)
            d->trigger(d->menu.action(info.id));
    }
}

auto MainWindow::onMousePressEvent(QMouseEvent *event) -> void
{
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

auto MainWindow::onWheelEvent(QWheelEvent *event) -> void
{
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

auto MainWindow::dragEnterEvent(QDragEnterEvent *event) -> void
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

auto MainWindow::dropEvent(QDropEvent *event) -> void
{
    const auto urls = event->mimeData()->urls();
    if (!event->mimeData()->hasUrls() || urls.isEmpty())
        return;
    Playlist playlist;
    QStringList subList;
    for (int i=0; i<urls.size(); ++i) {
        const auto suffix = QFileInfo(urls[i].path()).suffix().toLower();
        if (_IsSuffixOf(PlaylistExt, suffix)) {
            Playlist list;
            list.load(urls[i]);
            playlist += list;
        } else if (_IsSuffixOf(SubtitleExt, suffix))
            subList << urls[i].toLocalFile();
        else if (_IsSuffixOf(VideoExt, suffix) || _IsSuffixOf(AudioExt, suffix))
            playlist.append(urls[i]);
    }
    if (!playlist.isEmpty()) {
        d->openWith(d->pref().open_media_by_drag_and_drop, playlist);
    } else if (!subList.isEmpty())
        appendSubFiles(subList, true, d->pref().sub_enc);
}

auto MainWindow::reloadSkin() -> void
{
    d->player = nullptr;
    d->view->setSkin(d->pref().skin_name);
    auto root = d->view->rootObject();
    if (!root)
        return;
    if (root->objectName() == "player")
        d->player = qobject_cast<QQuickItem*>(root);
    if (!d->player)
        d->player = root->findChild<QQuickItem*>("player");
    if (d->player) {
        if (auto item = d->view->findItem("playinfo"))
            item->setProperty("show", d->as.playinfo_visible);
        if (auto item = d->view->findItem("logo")) {
            item->setProperty("show", d->pref().show_logo);
            item->setProperty("color", d->pref().bg_color);
        }
    }
}

auto MainWindow::applyPref() -> void
{
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
    const auto backend = p.enable_hwaccel ? p.hwaccel_backend : HwAcc::None;
    const auto codecs = p.enable_hwaccel ? p.hwaccel_codecs : QVector<int>();
    d->engine.setHwAcc(backend, codecs);
    AudioNormalizerOption normalizer;
    normalizer.bufferLengthInSeconds = p.normalizer_length;
    normalizer.targetLevel = p.normalizer_target;
    normalizer.minimumGain = p.normalizer_min;
    normalizer.maximumGain = p.normalizer_max;
    normalizer.silenceLevel = p.normalizer_silence;
    d->engine.setVolumeNormalizerOption(normalizer);
    d->engine.setImageDuration(p.image_duration);
    d->engine.setChannelLayoutMap(p.channel_manipulation);
    d->engine.setSubtitleStyle(p.sub_style);
    auto conv = [&p] (const DeintCaps &caps) {
        DeintOption option;
        option.method = caps.method();
        option.doubler = caps.doubler();
        if (caps.hwdec()) {
            if (!caps.supports(DeintDevice::GPU)
                    && !caps.supports(DeintDevice::OpenGL))
                return DeintOption();
            if (caps.supports(DeintDevice::GPU)
                    && p.hwdeints.contains(caps.method()))
                option.device = DeintDevice::GPU;
            else
                option.device = DeintDevice::OpenGL;
        } else
            option.device = caps.supports(DeintDevice::OpenGL)
                            ? DeintDevice::OpenGL : DeintDevice::CPU;
        return option;
    };
    const auto deint_swdec = conv(p.deint_swdec);
    const auto deint_hwdec = conv(p.deint_hwdec);
    d->engine.setDeintOptions(deint_swdec, deint_hwdec);
    d->engine.setAudioDriver(p.audio_driver);
    d->engine.setClippingMethod(p.clipping_method);
    d->engine.setMinimumCache(p.cache_min_playback, p.cache_min_seeking);
    Kernel3x3 blur, sharpen;
    blur.setCenter(p.blur_kern_c);
    blur.setNeighbor(p.blur_kern_n);
    blur.setDiagonal(p.blur_kern_d);
    sharpen.setCenter(p.sharpen_kern_c);
    sharpen.setNeighbor(p.sharpen_kern_n);
    sharpen.setDiagonal(p.sharpen_kern_d);
    d->renderer.setKernel(blur, sharpen);
    SubtitleParser::setMsPerCharactor(p.ms_per_char);
    d->subtitle.setPriority(p.sub_priority);
    d->subtitle.setStyle(p.sub_style);

    d->menu.retranslate();
    d->menu.setShortcuts(p.shortcuts);
    d->menu("play")("speed").s()->setStep(p.speed_step);
    d->menu("play")("seek").s("seek1")->setStep(p.seek_step1);
    d->menu("play")("seek").s("seek2")->setStep(p.seek_step2);
    d->menu("play")("seek").s("seek3")->setStep(p.seek_step3);
    d->menu("subtitle")("position").s()->setStep(p.sub_pos_step);
    d->menu("subtitle")("sync").s()->setStep(p.sub_sync_step);
    d->menu("video")("color").s("brightness")->setStep(p.brightness_step);
    d->menu("video")("color").s("contrast")->setStep(p.contrast_step);
    d->menu("video")("color").s("saturation")->setStep(p.saturation_step);
    d->menu("video")("color").s("hue")->setStep(p.hue_step);
    d->menu("audio")("sync").s()->setStep(p.audio_sync_step);
    d->menu("audio")("volume").s()->setStep(p.volume_step);
    d->menu("audio")("amp").s()->setStep(p.amp_step);
    d->menu.syncTitle();
    d->menu.resetKeyMap();

    cApp.setHeartbeat(p.use_heartbeat ? p.heartbeat_command : QString(),
                      p.heartbeat_interval);

    d->theme.setOsd(p.osd_theme);
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

template<class Slot>
void connectCopies(Menu &menu, const Slot &slot) {
    QObject::connect(&menu, &Menu::aboutToShow, slot);
    for (QMenu *copy : menu.copies()) {
        QObject::connect(copy, &QMenu::aboutToShow, slot);
    }
}

auto MainWindow::resizeEvent(QResizeEvent *event) -> void
{
    QWidget::resizeEvent(event);
    if (!d->fullScreen)
        d->updateWindowSizeState();
}

auto MainWindow::onKeyPressEvent(QKeyEvent *event) -> void
{
    QWidget::keyPressEvent(event);
    constexpr int modMask = Qt::SHIFT | Qt::CTRL | Qt::ALT | Qt::META;
    const QKeySequence shortcut(event->key() + (event->modifiers() & modMask));
    d->trigger(RootMenu::instance().action(shortcut));
    event->accept();
}


auto MainWindow::doVisibleAction(bool visible) -> void
{
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
        if (!d->engine.isPlaying()
                || (d->pref().pause_video_only && !d->engine.hasVideo()))
            return;
        d->pausedByHiding = true;
        d->engine.pause();
    }
}

auto MainWindow::showEvent(QShowEvent *event) -> void
{
    QWidget::showEvent(event);
    doVisibleAction(true);
}

auto MainWindow::hideEvent(QHideEvent *event) -> void
{
    QWidget::hideEvent(event);
    doVisibleAction(false);
}

auto MainWindow::changeEvent(QEvent *ev) -> void
{
    QWidget::changeEvent(ev);
    if (ev->type() == QEvent::WindowStateChange) {
        auto event = static_cast<QWindowStateChangeEvent*>(ev);
        d->prevWinState = event->oldState();
        d->winState = windowState();
        d->updateWindowSizeState();
        setWindowFilePath(d->filePath);
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
            doVisibleAction(d->winState != Qt::WindowMinimized);
    }
}

auto MainWindow::closeEvent(QCloseEvent *event) -> void
{
    QWidget::closeEvent(event);
#ifndef Q_OS_MAC
    if (d->tray && d->pref().enable_system_tray
            && d->pref().hide_rather_close) {
        hide();
        if (d->as.ask_system_tray) {
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
            d->as.ask_system_tray = !mbox.checkBox()->isChecked();
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

auto MainWindow::updateStaysOnTop() -> void
{
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

auto MainWindow::updateTitle() -> void
{
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

auto MainWindow::updateMrl(const Mrl &mrl) -> void
{
    updateTitle();
    const auto disc = mrl.isDisc();
    d->playlist.setLoaded(mrl);
    auto menu = d->menu("play")["disc-menu"];
    menu->setEnabled(disc);
    menu->setVisible(disc);
}

auto MainWindow::clearSubtitleFiles() -> void
{
    d->subtitle.unload();
    qDeleteAll(d->menu("subtitle")("track").g("external")->actions());
    for (auto action : d->menu("subtitle")("track").g("internal")->actions()) {
        auto id = action->data().toInt();
        if (d->engine.subtitleStreams()[id].isExternal())
            d->engine.removeSubtitleStream(id);
    }
}

auto MainWindow::load(Subtitle &sub, const QString &fileName,
                      const QString &encoding) -> bool
{
    const auto autodet = d->pref().sub_enc_autodetection;
    const auto accuracy = autodet ? d->pref().sub_enc_accuracy*0.01 : -1.0;
    if (sub.load(fileName, encoding, accuracy))
        return true;
    d->engine.addSubtitleStream(fileName, encoding);
    return false;
}

auto MainWindow::appendSubFiles(const QStringList &files,
                                bool checked, const QString &enc) -> void
{
    if (!files.isEmpty()) {
        Subtitle sub;
        for (auto file : files) {
            if (load(sub, file, enc))
                d->subtitle.load(sub, checked);
        }
        d->syncSubtitleFileMenu();
    }
}

auto MainWindow::moveEvent(QMoveEvent *event) -> void
{
    QWidget::moveEvent(event);
    if (!d->fullScreen)
        d->updateWindowPosState();
}

void MainWindow::showMessage(const QString &cmd, int value,
                             const QString &unit, bool sign)
{
    showMessage(cmd, _SignN(value, sign) + unit);
}

void MainWindow::showMessage(const QString &cmd, double value,
                             const QString &unit, bool sign)
{
    showMessage(cmd, _SignN(value, sign) + unit);
}
