#include "mainwindow_p.hpp"
#include "app.hpp"
#include "misc/trayicon.hpp"
#include "misc/stepactionpair.hpp"
#include "video/kernel3x3.hpp"
#include "video/deintoption.hpp"
#include "video/videoformat.hpp"
#include "audio/audionormalizeroption.hpp"
#include "subtitle/subtitleview.hpp"
#include "subtitle/subtitlemodel.hpp"
#include "subtitle/subtitle_parser.hpp"
#include "dialog/mbox.hpp"
#include "dialog/openmediafolderdialog.hpp"

MainWindow::Data::Data(MainWindow *p)
    : p(p)
{
    preferences.load();
}

template<class List>
auto MainWindow::Data::updateListMenu(Menu &menu, const List &list,
                                      int current, const QString &group) -> void
{
    if (group.isEmpty())
        menu.setEnabledSync(!list.isEmpty());
    if (!list.isEmpty()) {
        menu.g(group)->clear();
        for (auto it = list.begin(); it != list.end(); ++it) {
            auto act = menu.addActionToGroupWithoutKey(it->name(), true, group);
            act->setData(it->id());
            if (current == it->id())
                act->setChecked(true);
        }
    } else if (!group.isEmpty()) // partial in menu
        menu.g(group)->clear();
    menu.syncActions();
}

auto MainWindow::Data::initContextMenu() -> void
{
    auto addContextMenu = [this] (Menu &menu)
        { contextMenu.addMenu(menu.copied(&contextMenu)); };
    addContextMenu(menu(u"open"_q));
    contextMenu.addSeparator();
    addContextMenu(menu(u"play"_q));
    addContextMenu(menu(u"video"_q));
    addContextMenu(menu(u"audio"_q));
    addContextMenu(menu(u"subtitle"_q));
    contextMenu.addSeparator();
    addContextMenu(menu(u"tool"_q));
    addContextMenu(menu(u"window"_q));
    contextMenu.addSeparator();
    contextMenu.addAction(menu(u"help"_q)[u"about"_q]);
    contextMenu.addAction(menu[u"exit"_q]);
#ifdef Q_OS_MAC
////    qt_mac_set_dock_menu(&menu);
    QMenuBar *mb = cApp.globalMenuBar();
    qDeleteAll(mb->actions());
    auto addMenuBar = [this, mb] (Menu &menu)
        {mb->addMenu(menu.copied(mb));};
    addMenuBar(menu(u"open"_q));
    addMenuBar(menu(u"play"_q));
    addMenuBar(menu(u"video"_q));
    addMenuBar(menu(u"audio"_q));
    addMenuBar(menu(u"subtitle"_q));
    addMenuBar(menu(u"tool"_q));
    addMenuBar(menu(u"window"_q));
    addMenuBar(menu(u"help"_q));
#endif
}

auto MainWindow::Data::initWidget() -> void
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
        sgInit = true;
        emit p->sceneGraphInitialized();
    }, Qt::DirectConnection);
    connect(view, &QQuickView::sceneGraphInvalidated, p, [this] () {
        sgInit = false;
        auto context = QOpenGLContext::currentContext();
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

auto MainWindow::Data::initEngine() -> void
{
    engine.setVideoRenderer(&vr);
    connect(&engine, &PlayEngine::mrlChanged,
            p, [=] (const Mrl &mrl) { updateMrl(mrl); });
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
            menu(u"play"_q)[u"pause"_q]->setText(tr("Pause"));
            break;
        default:
            menu(u"play"_q)[u"pause"_q]->setText(tr("Play"));
        }
        const auto disable = pref().disable_screensaver
                             && state == PlayEngine::Playing;
        cApp.setScreensaverDisabled(disable);
        updateStaysOnTop();
        stateChanging = false;
    });
    connect(&engine, &PlayEngine::tick, p, [this] (int time) {
        if (ab.check(time))
            engine.seek(ab.a());
        else
            subtitle.render(time);
    });
    connect(&engine, &PlayEngine::volumeChanged,
            p, [this] (int volume) {
        _Change(as.state.audio_volume, volume);
    });
    connect(&engine, &PlayEngine::volumeNormalizerActivatedChanged,
            menu(u"audio"_q)[u"normalizer"_q], &QAction::setChecked);
    connect(&engine, &PlayEngine::tempoScaledChanged,
            menu(u"audio"_q)[u"tempo-scaler"_q], &QAction::setChecked);
    connect(&engine, &PlayEngine::mutedChanged,
            menu(u"audio"_q)(u"volume"_q)[u"mute"_q], &QAction::setChecked);
    connect(&engine, &PlayEngine::fpsChanged,
            &subtitle, &SubtitleRendererItem::setFPS);
    connect(&engine, &PlayEngine::editionsChanged,
            p, [this] (const EditionList &editions) {
        const auto edition = engine.currentEdition();
        updateListMenu(menu(u"play"_q)(u"title"_q), editions, edition);
    });
    connect(&engine, &PlayEngine::audioStreamsChanged,
            p, [this] (const StreamList &streams) {
        const auto stream = engine.currentAudioStream();
        updateListMenu(menu(u"audio"_q)(u"track"_q), streams, stream);
    });
    connect(&engine, &PlayEngine::videoStreamsChanged,
            p, [this] (const StreamList &streams) {
        const auto stream = engine.currentVideoStream();
        updateListMenu(menu(u"video"_q)(u"track"_q), streams, stream);
    });
    connect(&engine, &PlayEngine::subtitleStreamsChanged,
            p, [this] (const StreamList &streams) {
        auto &menu = this->menu(u"subtitle"_q)(u"track"_q);
        const auto stream = engine.currentSubtitleStream();
        updateListMenu(menu, streams, stream, u"internal"_q);
    });
    connect(&engine, &PlayEngine::chaptersChanged,
            p, [this] (const ChapterList &chapters) {
        const auto chapter = engine.currentChapter();
        updateListMenu(menu(u"play"_q)(u"chapter"_q), chapters, chapter);
    });
    connect(&engine, &PlayEngine::currentAudioStreamChanged,
            p, [this] (int stream) {
        auto action = menu(u"audio"_q)(u"track"_q).g()->find(stream);
        if (action && !action->isChecked()) {
            action->setChecked(true);
            menu(u"audio"_q)(u"track"_q).syncActions();
        }
    });
    connect(&engine, &PlayEngine::currentVideoStreamChanged,
            p, [this] (int stream) {
        auto action = menu(u"video"_q)(u"track"_q).g()->find(stream);
        if (action && !action->isChecked()) {
            action->setChecked(true);
            menu(u"video"_q)(u"track"_q).syncActions();
        }
    });
    connect(&engine, &PlayEngine::currentSubtitleStreamChanged,
            p, [this] (int stream) {
        auto acts = menu(u"subtitle"_q)(u"track"_q).g(u"internal"_q)->actions();
        for (auto action : acts)
            action->setChecked(action->data().toInt() == stream);
        menu(u"subtitle"_q)(u"track"_q).syncActions();
    });

    connect(&engine, &PlayEngine::started, p, [this] (Mrl mrl) {
        starting = true;
        setOpen(mrl);
        as.state.mrl = mrl.toUnique();
        auto &state = as.state;
        state.sub_track = SubtitleStateInfo();
        const bool found = history.getState(&state);
        as.state.mrl = mrl;
        if (found) {
            engine.setCurrentAudioStream(state.audio_track, true);
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
        starting = false;
    });
    connect(&engine, &PlayEngine::finished,
            p, [this] (const FinishInfo &info) {
        as.state.mrl = info.mrl.toUnique();
        as.state.device = info.mrl.device();
        as.state.last_played_date_time = QDateTime::currentDateTime();
        as.state.resume_position = info.remain > 500 ? info.position : -1;
        as.state.edition = engine.currentEdition();
        as.state.audio_track = info.streamIds[StreamAudio];
        as.state.sub_track = subtitleState();
        as.state.sub_track.setTrack(info.streamIds[StreamSubtitle]);
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

auto MainWindow::Data::initItems() -> void
{
    connect(&recent, &RecentInfo::openListChanged,
            p, [=] (const QList<Mrl> &list) { updateRecentActions(list); });
    connect(&hider, &QTimer::timeout,
            p, [this] () { view->setCursorVisible(false); });
    connect(&history, &HistoryModel::playRequested,
            p, [this] (const Mrl &mrl) { openMrl(mrl); });
    connect(&playlist, &PlaylistModel::playRequested,
            p, [this] (int row) { openMrl(playlist.at(row)); });
    connect(&playlist, &PlaylistModel::finished, p, [this] () {
        if (menu(u"tool"_q)[u"auto-exit"_q]->isChecked()) p->exit();
        if (menu(u"tool"_q)[u"auto-shutdown"_q]->isChecked()) cApp.shutdown();
    });
    connect(&subtitle, &SubtitleRendererItem::modelsChanged,
            subtitleView, &SubtitleView::setModels);

    vr.setOverlay(&subtitle);
    auto showSize = [this] {
        const auto num = [] (qreal n) { return _N(qRound(n)); };
        const auto w = num(vr.width()), h = num(vr.height());
        showMessage(w % u'×'_q % h, &pref().show_osd_on_resized);
    };
    connect(&vr, &VideoRenderer::sizeChanged, p, showSize);
}

auto MainWindow::Data::initTimers() -> void
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
            p, [=] () { applyPref(); cApp.runCommands(); });
    initializer.start(1);
}

auto MainWindow::Data::resume(const Mrl &mrl, int *edition) -> int
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
    QString time = _MSecToString(state->resume_position, u"h:mm:ss"_q);
    if (state->edition >= 0)
        time += '['_q % tr("Title %1").arg(state->edition) % ']'_q;
    const auto date = state->last_played_date_time.toString(Qt::ISODate);
    mbox.setInformativeText(tr("Played Date: %1\nStopped Position: %2")
                            .arg(date).arg(time));
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

auto MainWindow::Data::cache(const Mrl &mrl) -> int
{
    if (mrl.isLocalFile()) {
        auto path = mrl.toLocalFile();
        for (auto &folder : pref().network_folders) {
            if (path.startsWith(folder))
                return pref().cache_network;
        }
        return pref().cache_local;
    } if (mrl.isDisc())
        return pref().cache_disc;
    return pref().cache_network;
}

auto MainWindow::Data::tryToAutoselect(const QVector<SubComp> &loaded,
                                       const Mrl &mrl) -> QVector<int>
{
    QVector<int> selected;
    const auto &p = pref();
    if (loaded.isEmpty() || !p.sub_enable_autoselect)
        return selected;
    QSet<QString> langSet;
    const QFileInfo file(mrl.toLocalFile());
    const QString base = file.completeBaseName();
    for (int i=0; i<loaded.size(); ++i) {
        bool select = false;
        switch (p.sub_autoselect) {
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
    if (p.sub_autoselect == SubtitleAutoselect::Matched
            && !selected.isEmpty() && !p.sub_ext.isEmpty()) {
        for (int i=0; i<selected.size(); ++i) {
            const auto fileName = loaded[selected[i]].fileName();
            const auto suffix = QFileInfo(fileName).suffix();
            if (p.sub_ext == suffix.toLower()) {
                const int idx = selected[i];
                selected.clear();
                selected.append(idx);
                break;
            }
        }
    }
    return selected;
}

auto MainWindow::Data::tryToAutoload(const Mrl &mrl,
                                     const QString &path) -> QVector<SubComp>
{
    QVector<SubComp> loaded;
    const auto &p = pref();
    if (!p.sub_enable_autoload)
        return loaded;
    const QFileInfo fileInfo(mrl.toLocalFile());
    auto dir = fileInfo.dir();
    if (!path.isEmpty() && !dir.cd(path))
        return loaded;
    static const auto filter = _ToNameFilter(SubtitleExt);
    const auto all = dir.entryInfoList(filter, QDir::Files, QDir::Name);
    const auto base = fileInfo.completeBaseName();
    for (int i=0; i<all.size(); ++i) {
        if (p.sub_autoload != SubtitleAutoload::Folder) {
            if (p.sub_autoload == SubtitleAutoload::Matched) {
                if (base != all[i].completeBaseName())
                    continue;
            } else if (!all[i].fileName().contains(base))
                continue;
        }
        Subtitle sub;
        if (load(sub, all[i].absoluteFilePath(), p.sub_enc)) {
            for (int i=0; i<sub.size(); ++i)
                loaded.push_back(sub[i]);
        }
    }
    return loaded;
}

auto MainWindow::Data::updateSubtitleState() -> void
{
    const auto &mrl = as.state.mrl;
    if (mrl.isLocalFile()) {
        auto loaded = tryToAutoload(mrl);
        for (auto &path : pref().sub_search_paths)
            loaded += tryToAutoload(mrl, path);
        const auto selected = tryToAutoselect(loaded, mrl);
        for (int i=0; i<selected.size(); ++i)
            loaded[selected[i]].selection() = true;
        subtitle.setComponents(loaded);
    } else
        clearSubtitleFiles();
    syncSubtitleFileMenu();
}

auto MainWindow::Data::openWith(const OpenMediaInfo &mode,
                                const QList<Mrl> &mrls) -> void
{
    if (mrls.isEmpty())
        return;
    const auto mrl = mrls.first();
    auto checkAndPlay = [this] (const Mrl &mrl) {
        if (mrl != engine.mrl())
            return false;
        if (!engine.isPlaying())
            load(mrl);
        return true;
    };
    if (!checkAndPlay(mrl)) {
        Playlist pl;
        switch (mode.behavior) {
        case OpenMediaBehavior::Append:
            pl.append(mrls);
            break;
        case OpenMediaBehavior::ClearAndAppend:
            pl.clear();
            pl.append(mrls);
            break;
        case OpenMediaBehavior::NewPlaylist:
            pl.clear();
            pl = generatePlaylist(mrl);
            break;
        }
        playlist.merge(pl);
        load(mrl, mode.start_playback);
        if (!mrl.isDvd())
            recent.stack(mrl);
    }
    if (!p->isVisible())
        p->show();
}

auto MainWindow::Data::setVideoSize(const QSize &video) -> void
{
    if (p->isFullScreen() || p->isMaximized())
        return;
    // patched by Handrake
    const QSizeF screen = screenSize();
    const QSize size = (p->size() - vr.size().toSize() + video);
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

auto MainWindow::Data::syncState() -> void
{
    for (auto &eg : enumGroups) {
        Q_ASSERT(as.state.property(eg.property).isValid());
        auto data = eg.group->data();
        if (data.isValid())
            as.state.setProperty(eg.property, data);
    }
}

auto MainWindow::Data::syncWithState() -> void
{
    for (auto &eg : enumGroups)
        eg.group->setChecked(as.state.property(eg.property), true);
}

auto MainWindow::Data::trigger(QAction *action) -> void
{
    if (!action)
        return;
    if (view->topLevelItem()->isVisible()) {
        if (unblockedActions.isEmpty()) {
            unblockedActions += menu(u"window"_q).actions();
            qSort(unblockedActions);
        }
        const auto it = qBinaryFind(_C(unblockedActions), action);
        if (it == unblockedActions.cend())
            return;
    }
    action->trigger();
}

auto MainWindow::Data::lastCheckedSubtitleIndex() const -> int
{
    auto &list = menu(u"subtitle"_q)(u"track"_q);
    const auto internal = list.g(u"internal"_q)->actions();
    const auto external = list.g(u"external"_q)->actions();
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

auto MainWindow::Data::setSubtitleTracksToEngine() -> void
{
    StreamList track;
    for (auto comp : subtitle.components()) {
        if (comp->selection())
            track[comp->id()] = StreamTrack::fromSubComp(*comp);
    }
    engine.setSubtitleFiles(track);
}

auto MainWindow::Data::syncSubtitleFileMenu() -> void
{
    if (changingSub)
        return;
    changingSub = true;
    auto &list = menu(u"subtitle"_q)(u"track"_q);
    auto g = list.g(u"external"_q);
    const auto components = subtitle.components();
    while (g->actions().size() < components.size()) {
        auto action = g->addAction(u""_q);
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

auto MainWindow::Data::subtitleState() const -> SubtitleStateInfo
{
    SubtitleStateInfo state;
    state.setTrack(engine.currentSubtitleStream());
    state.mpv() = engine.subtitleFiles();
    const auto parsed = subtitle.components();
    for (auto c : parsed)
        state.append(*c);
    return state;
}

auto MainWindow::Data::setSubtitleState(const SubtitleStateInfo &state) -> void
{
    if (!state.isValid())
        return;
    for (auto &f : state.mpv())
        engine.addSubtitleStream(f.path, f.encoding);
    auto loaded = state.load();
    subtitle.setComponents(loaded);
    engine.setCurrentSubtitleStream(state.getTrack(), starting);
    syncSubtitleFileMenu();
}

auto MainWindow::Data::readyToHideCursor() -> void
{
    if (pref().hide_cursor
            && (p->isFullScreen() || !pref().hide_cursor_fs_only))
        hider.start(pref().hide_cursor_delay);
    else
        cancelToHideCursor();
}

auto MainWindow::Data::commitData() -> void
{
    static bool first = true;
    if (first) {
        recent.setLastPlaylist(playlist.list());
        recent.setLastMrl(engine.mrl());
        engine.shutdown();
        if (!p->isFullScreen())
            updateWindowPosState();
        as.state.video_effects = vr.effects();
        as.playlist_visible = playlist.isVisible();
        as.playlist_shuffled = playlist.isShuffled();
        as.playlist_repeat = playlist.repeat();
        as.history_visible = history.isVisible();
        as.save();
        syncState();
        engine.waitUntilTerminated();
        cApp.processEvents();
        first = false;
    }
}

auto MainWindow::Data::showTimeLine() -> void
{
    if (player && pref().show_osd_timeline)
        QMetaObject::invokeMethod(player, "showTimeLine");
}
auto MainWindow::Data::showMessageBox(const QVariant &msg) -> void
{
    if (player)
        QMetaObject::invokeMethod(player, "showMessageBox", Q_ARG(QVariant, msg));
}
auto MainWindow::Data::showOSD(const QVariant &msg) -> void
{
    if (player)
        QMetaObject::invokeMethod(player, "showOSD", Q_ARG(QVariant, msg));
}

auto MainWindow::Data::updateWindowSizeState() -> void
{
    if (!p->isFullScreen() && !p->isMinimized() && p->isVisible())
        as.win_size = p->size();
}

auto MainWindow::Data::screenSize() const -> QSize
{
    if (desktop->isVirtualDesktop())
        return virtualDesktopSize;
    return desktop->availableGeometry(p).size();
}

auto MainWindow::Data::updateWindowPosState() -> void
{
    if (!p->isFullScreen() && !p->isMinimized() && p->isVisible()) {
        const auto screen = screenSize();
        as.win_pos.rx() = qBound(0.0, p->x()/(double)screen.width(), 1.0);
        as.win_pos.ry() = qBound(0.0, p->y()/(double)screen.height(), 1.0);
    }
}

auto MainWindow::Data::openDir(const QString &dir) -> void
{
    OpenMediaFolderDialog dlg(p);
    dlg.setFolder(dir);
    if (dlg.exec()) {
        const auto list = dlg.playlist();
        if (!list.isEmpty()) {
            playlist.setList(list);
            load(list.first());
            recent.stack(list.first());
        }
    }
}

auto MainWindow::Data::openMrl(const Mrl &mrl) -> void
{
    if (mrl == engine.mrl()) {
        if (!engine.startInfo().isValid())
            load(mrl);
    } else {
        if (playlist.rowOf(mrl) < 0)
            playlist.setList(generatePlaylist(mrl));
        load(mrl);
        if (!mrl.isDvd())
            recent.stack(mrl);
    }
}

auto MainWindow::Data::generatePlaylist(const Mrl &mrl) const -> Playlist
{
    if (!mrl.isLocalFile() || !pref().enable_generate_playist)
        return Playlist(mrl);
    const auto mode = pref().generate_playlist;
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
        static QRegEx rxs(uR"((\D*)\d+(.*))"_q);
        const auto ms = rxs.match(fileName);
        if (!ms.hasMatch())
            continue;
        static QRegEx rxt(uR"((\D*)\d+(.*))"_q);
        const auto mt = rxt.match(it->fileName());
        if (!mt.hasMatch())
            continue;
        if (!prefix && !suffix) {
            if (ms.capturedRef(1) == mt.capturedRef(1))
                prefix = true;
            else if (ms.capturedRef(2) == mt.capturedRef(2))
                suffix = true;
            else
                continue;
        } else if (prefix) {
            if (ms.capturedRef(1) != mt.capturedRef(1))
                continue;
        } else if (suffix) {
            if (ms.capturedRef(2) != mt.capturedRef(2))
                continue;
        }
        list.append(it->absoluteFilePath());
    }
    if (list.isEmpty())
        return Playlist(mrl);
    return list;
}

auto MainWindow::Data::showMessage(const QString &msg, const bool *force) -> void
{
    if (force) {
        if (!*force)
            return;
    } else if (!pref().show_osd_on_action)
        return;
    if (!dontShowMsg)
        showOSD(msg);
}

SIA _SignN(int value, bool sign) -> QString
    { return sign ? _NS(value) : _N(value); }

SIA _SignN(double value, bool sign, int n = 1) -> QString
    { return sign ? _NS(value, n) : _N(value, n); }

void MainWindow::Data::showMessage(const QString &cmd, int value,
                             const QString &unit, bool sign)
{
    showMessage(cmd, _SignN(value, sign) + unit);
}

void MainWindow::Data::showMessage(const QString &cmd, double value,
                             const QString &unit, bool sign)
{
    showMessage(cmd, _SignN(value, sign) + unit);
}

auto MainWindow::Data::appendSubFiles(const QStringList &files,
                                      bool checked, const QString &enc) -> void
{
    if (!files.isEmpty()) {
        Subtitle sub;
        for (auto file : files) {
            if (load(sub, file, enc))
                subtitle.load(sub, checked);
        }
        syncSubtitleFileMenu();
    }
}

auto MainWindow::Data::clearSubtitleFiles() -> void
{
    subtitle.unload();
    qDeleteAll(menu(u"subtitle"_q)(u"track"_q).g(u"external"_q)->actions());
    for (auto action : menu(u"subtitle"_q)(u"track"_q).g(u"internal"_q)->actions()) {
        auto id = action->data().toInt();
        if (engine.subtitleStreams()[id].isExternal())
            engine.removeSubtitleStream(id);
    }
}

auto MainWindow::Data::applyPref() -> void
{
    int time = -1, title = -1;
    switch (engine.state()) {
    case PlayEngine::Playing:
    case PlayEngine::Loading:
    case PlayEngine::Paused:
        time = engine.time();
        title = engine.currentEdition();
        break;
    default:
        break;
    }
    auto &p = pref();
    youtube->setUserAgent(p.yt_user_agent);
    youtube->setProgram(p.yt_program);
    history.setRememberImage(p.remember_image);
    history.setPropertiesToRestore(p.restore_properties);
    const auto backend = p.enable_hwaccel ? p.hwaccel_backend : HwAcc::None;
    const auto codecs = p.enable_hwaccel ? p.hwaccel_codecs : QVector<int>();
    engine.setHwAcc(backend, codecs);
    AudioNormalizerOption normalizer;
    normalizer.bufferLengthInSeconds = p.normalizer_length;
    normalizer.targetLevel = p.normalizer_target;
    normalizer.minimumGain = p.normalizer_min;
    normalizer.maximumGain = p.normalizer_max;
    normalizer.silenceLevel = p.normalizer_silence;
    engine.setVolumeNormalizerOption(normalizer);
    engine.setChannelLayoutMap(p.channel_manipulation);
    engine.setSubtitleStyle(p.sub_style);
    engine.setSubtitlePriority(p.sub_priority);
    engine.setAudioPriority(p.audio_priority);
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
    engine.setDeintOptions(deint_swdec, deint_hwdec);
    engine.setAudioDevice(p.audio_device);
    engine.setClippingMethod(p.clipping_method);
    engine.setMinimumCache(p.cache_min_playback/100., p.cache_min_seeking/100.);
    Kernel3x3 blur, sharpen;
    blur.setCenter(p.blur_kern_c);
    blur.setNeighbor(p.blur_kern_n);
    blur.setDiagonal(p.blur_kern_d);
    sharpen.setCenter(p.sharpen_kern_c);
    sharpen.setNeighbor(p.sharpen_kern_n);
    sharpen.setDiagonal(p.sharpen_kern_d);
    vr.setKernel(blur, sharpen);
    SubtitleParser::setMsPerCharactor(p.ms_per_char);
    subtitle.setPriority(p.sub_priority);
    subtitle.setStyle(p.sub_style);

    menu.retranslate();
    menu.setShortcuts(p.shortcuts);
    menu(u"play"_q)(u"speed"_q).s()->setStep(p.speed_step);
    menu(u"play"_q)(u"seek"_q).s(u"seek1"_q)->setStep(p.seek_step1);
    menu(u"play"_q)(u"seek"_q).s(u"seek2"_q)->setStep(p.seek_step2);
    menu(u"play"_q)(u"seek"_q).s(u"seek3"_q)->setStep(p.seek_step3);
    menu(u"subtitle"_q)(u"position"_q).s()->setStep(p.sub_pos_step);
    menu(u"subtitle"_q)(u"sync"_q).s()->setStep(p.sub_sync_step);
    menu(u"video"_q)(u"color"_q).s(u"brightness"_q)->setStep(p.brightness_step);
    menu(u"video"_q)(u"color"_q).s(u"contrast"_q)->setStep(p.contrast_step);
    menu(u"video"_q)(u"color"_q).s(u"saturation"_q)->setStep(p.saturation_step);
    menu(u"video"_q)(u"color"_q).s(u"hue"_q)->setStep(p.hue_step);
    menu(u"audio"_q)(u"sync"_q).s()->setStep(p.audio_sync_step);
    menu(u"audio"_q)(u"volume"_q).s()->setStep(p.volume_step);
    menu(u"audio"_q)(u"amp"_q).s()->setStep(p.amp_step);
    menu.syncTitle();
    menu.resetKeyMap();

    cApp.setHeartbeat(p.use_heartbeat ? p.heartbeat_command : QString(),
                      p.heartbeat_interval);

    theme.setOsd(p.osd_theme);
    reloadSkin();

    if (time >= 0) {
        auto info = engine.startInfo();
        info.resume = time;
        info.cache = cache(info.mrl);
        if (info.mrl.isDisc())
            info.mrl = info.mrl.titleMrl(title);
        engine.load(info);
    }

    if (tray)
        tray->setVisible(p.enable_system_tray);
    preferences.save();
    cApp.setMprisActivated(preferences.use_mpris2);
}

auto MainWindow::Data::updateStaysOnTop() -> void
{
    if (p->windowState() & Qt::WindowMinimized)
        return;
    sotChanging = true;
    const auto id = as.win_stays_on_top;
    bool onTop = false;
    if (!p->isFullScreen()) {
        if (id == StaysOnTop::Always)
            onTop = true;
        else if (id == StaysOnTop::None)
            onTop = false;
        else
            onTop = engine.isPlaying();
    }
    cApp.setAlwaysOnTop(p, onTop);
    sotChanging = false;
}

auto MainWindow::Data::setVideoSize(double rate) -> void
{
    if (rate < 0) {
        p->setFullScreen(!p->isFullScreen());
    } else {
        if (p->isFullScreen())
            p->setFullScreen(false);
        if (p->isMaximized())
            p->showNormal();
        const QSizeF video = vr.sizeHint();
        auto area = [] (const QSizeF &s) { return s.width()*s.height(); };
        if (rate == 0.0)
            rate = area(screenSize())*0.15/area(video);
        setVideoSize((video*qSqrt(rate)).toSize());
    }
}

auto MainWindow::Data::load(const Mrl &mrl, bool play) -> void
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

auto MainWindow::Data::load(Subtitle &sub, const QString &fileName,
                      const QString &encoding) -> bool
{
    const auto autodet = pref().sub_enc_autodetection;
    const auto accuracy = autodet ? pref().sub_enc_accuracy*0.01 : -1.0;
    if (sub.load(fileName, encoding, accuracy))
        return true;
    engine.addSubtitleStream(fileName, encoding);
    return false;
}

auto MainWindow::Data::reloadSkin() -> void
{
    player = nullptr;
    view->setSkin(pref().skin_name);
    auto root = view->rootObject();
    if (!root)
        return;
    if (root->objectName() == "player"_a)
        player = qobject_cast<QQuickItem*>(root);
    if (!player)
        player = root->findChild<QQuickItem*>(u"player"_q);
    if (player) {
        if (auto item = view->findItem(u"playinfo"_q))
            item->setProperty("show", as.playinfo_visible);
        if (auto item = view->findItem(u"logo"_q)) {
            item->setProperty("show", pref().show_logo);
            item->setProperty("color", pref().bg_color);
        }
    }
}

auto MainWindow::Data::updateRecentActions(const QList<Mrl> &list) -> void
{
    Menu &recent = menu(u"open"_q)(u"recent"_q);
    ActionGroup *group = recent.g();
    const int diff = group->actions().size() - list.size();
    if (diff < 0) {
        const auto acts = recent.actions();
        QAction *sprt = acts[acts.size()-2];
        Q_ASSERT(sprt->isSeparator());
        for (int i=0; i<-diff; ++i) {
            QAction *action = new QAction(&recent);
            recent.insertAction(sprt, action);
            recent.g()->addAction(action);
        }
    } else if (diff > 0) {
        auto acts = recent.g()->actions();
        for (int i=0; i<diff; ++i)
            delete acts.takeLast();
    }
    const auto acts = group->actions();
    for (int i=0; i<list.size(); ++i) {
        QAction *act = acts[i];
        act->setData(list[i].location());
        act->setText(list[i].displayName());
        act->setVisible(!list[i].isEmpty());
    }
    recent.syncActions();
}

auto MainWindow::Data::updateMrl(const Mrl &mrl) -> void
{
    updateTitle();
    const auto disc = mrl.isDisc();
    playlist.setLoaded(mrl);
    auto action = menu(u"play"_q)[u"disc-menu"_q];
    action->setEnabled(disc);
    action->setVisible(disc);
}

auto MainWindow::Data::updateTitle() -> void
{
    const auto mrl = engine.mrl();
    p->setWindowFilePath(QString());
    QString fileName;
    if (!mrl.isEmpty()) {
        if (mrl.isLocalFile()) {
            const QFileInfo file(mrl.toLocalFile());
            filePath = file.absoluteFilePath();
            fileName = file.fileName();
            if (p->isVisible())
                p->setWindowFilePath(filePath);
        } else
            fileName = engine.mediaName();
    }
    cApp.setWindowTitle(p, fileName);
}

auto MainWindow::Data::doVisibleAction(bool visible) -> void
{
    this->visible = visible;
    if (visible) {
        if (pausedByHiding && engine.isPaused()) {
            engine.unpause();
            pausedByHiding = false;
        }
        p->setWindowFilePath(filePath);
#ifndef Q_OS_MAC
        p->setWindowIcon(cApp.defaultIcon());
#endif
    } else {
        const auto &p = pref();
        if (!p.pause_minimized || dontPause)
            return;
        if (!engine.isPlaying() || (p.pause_video_only && !engine.hasVideo()))
            return;
        pausedByHiding = true;
        engine.pause();
    }
}

auto MainWindow::Data::checkWindowState(Qt::WindowStates prev) -> void
{
    prevWinState = prev;
    winState = p->windowState();
    updateWindowSizeState();
    p->setWindowFilePath(filePath);
    dontPause = true;
    moving = false;
    prevPos = QPoint();
    const auto full = p->isFullScreen();
    if (full) {
        cApp.setAlwaysOnTop(p, false);
        p->setVisible(true);
    } else {
        updateStaysOnTop();
        p->setVisible(true);
    }
    readyToHideCursor();
    dontPause = false;
    if (!stateChanging)
        doVisibleAction(winState != Qt::WindowMinimized);
}
