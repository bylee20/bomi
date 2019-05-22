#include "mainwindow_p.hpp"
#include "skin.hpp"
#include "app.hpp"
#include "misc/trayicon.hpp"
#include "misc/stepactionpair.hpp"
#include "tmp/algorithm.hpp"
#include "video/kernel3x3.hpp"
#include "video/deintoption.hpp"
#include "video/videoformat.hpp"
#include "video/videopreview.hpp"
#include "audio/audionormalizeroption.hpp"
#include "subtitle/subtitleviewer.hpp"
#include "subtitle/subtitlemodel.hpp"
#include "subtitle/subtitle_parser.hpp"
#include "dialog/mbox.hpp"
#include "dialog/openmediafolderdialog.hpp"
#include "dialog/subtitlefinddialog.hpp"
#include "dialog/encoderdialog.hpp"
#include "avinfoobject.hpp"
#include "misc/smbauth.hpp"
#include "misc/filenamegenerator.hpp"
#include <QSessionManager>
#include <QScreen>

auto MainWindow::Data::restoreState() -> void
{
    playlist.setVisible(as.playlist_visible);
    playlist.setRepeat(as.playlist_repeat);
    playlist.setShuffled(as.playlist_shuffled);
    playlist.setList(recent.lastPlaylist());
    if (pref.load_last() && !recent.lastMrl().isEmpty()) {
        load(recent.lastMrl(), false);
        setOpen(recent.lastMrl());
    }
    updateRecentActions(recent.openList());
    history.setVisible(as.history_visible);

    auto &tool = menu(u"tool"_q);
    auto &pl = tool(u"playlist"_q);
    pl[u"shuffle"_q]->setChecked(as.playlist_shuffled);
    pl[u"repeat"_q]->setChecked(as.playlist_repeat);
    tool[u"auto-exit"_q]->setChecked(as.auto_exit);
    const auto mo = as.metaObject();
    for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        const auto p = mo->property(i);
        if (p.hasNotifySignal())
            p.notifySignal().invoke(&as, QGenericArgument(p.typeName(),
                                                          p.read(&as).constData()));
    }

    as.state.set_video_tracks(StreamList(StreamVideo));
    as.state.set_audio_tracks(StreamList(StreamAudio));
    as.state.set_sub_tracks(StreamList(StreamSubtitle));
    as.state.set_sub_tracks_inclusive(StreamList(StreamInclusiveSubtitle));
    e.restore(&as.state);
}

auto MainWindow::Data::initContextMenu() -> void
{
    auto addContextMenu = [this] (Menu &menu) { contextMenu.addMenu(&menu); };
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
////    qt_macOS::set_dock_menu(&menu);
    QMenuBar *mb = cApp.globalMenuBar();
    qDeleteAll(mb->actions());
    auto addMenuBar = [=] (Menu &menu)
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

auto MainWindow::Data::initTray() -> void
{
    if (!TrayIcon::isAvailable())
        return;
    tray = new TrayIcon(cApp.defaultIcon(), p);
    tray->setVisible(pref.enable_system_tray());
    connect(tray, &TrayIcon::activated, p, [=] (auto reason) {
        if (reason == TrayIcon::Trigger)
            p->setVisible(!p->isVisible());
        else if (reason == TrayIcon::Context)
            contextMenu.exec(QCursor::pos());
        else if (reason == TrayIcon::Show)
            p->setVisible(true);
        else if (reason == TrayIcon::Quit)
            p->exit();
    });
}

auto MainWindow::Data::plugEngine() -> void
{
    connect(&e, &PlayEngine::mrlChanged, p, [=] (const Mrl &mrl) { updateMrl(mrl); });
    connect(&e, &PlayEngine::stateChanged, p, [this] (PlayEngine::State state) {
        stateChanging = true;
        if (state == PlayEngine::Error) {
            waiter.stop();
            showMessageBox(tr("Error!\nCannot open the media."));
        }
        if (state != PlayEngine::Paused)
            pausedByHiding = false;
#ifdef Q_OS_WIN
        auto prog = taskbar.progress();
        switch (state) {
        case PlayEngine::Error:
            prog->setRange(0, 1);
            prog->setValue(1);
        case PlayEngine::Stopped:
            prog->stop();
            break;
        case PlayEngine::Paused:
            prog->setPaused(true);
            break;
        default:
            prog->setRange(e.begin(), e.end());
            prog->setValue(e.time());
            prog->resume();
            break;
        }
#endif
        const auto playing = e.isPlaying();
        const auto running = e.isRunning();
        menu(u"video"_q)(u"track"_q).setEnabled(running);
        menu(u"audio"_q)(u"track"_q).setEnabled(running);
        menu(u"subtitle"_q)(u"track"_q).setEnabled(running);
        menu(u"video"_q)(u"aspect"_q)[u"increase"_q]->setEnabled(running);
        menu(u"video"_q)(u"aspect"_q)[u"decrease"_q]->setEnabled(running);
        OS::setScreensaverEnabled(!pref.disable_screensaver() || !playing);
        updateStaysOnTop();
        stateChanging = false;
    });
    connect(&e, &PlayEngine::waitingChanged, p, [=] (auto waiting) {
        if (waiting) { waiter.start(); }
        else { waiter.stop(); this->showMessageBox(QString()); }
#ifdef Q_OS_WIN
        auto prog = taskbar.progress();
        switch (waiting) {
        case PlayEngine::Loading:
        case PlayEngine::Buffering:
            prog->setRange(0, 0);
            prog->resume();
            break;
        default:
            if (prog->maximum() == 0) {
                prog->setRange(e.begin(), e.end());
                prog->setValue(e.time());
                prog->resume();
            }
        }
#endif
    });
#ifdef Q_OS_WIN
    connect(&e, &PlayEngine::beginChanged, p, [=] () {
        taskbar.progress()->setRange(e.begin(), e.end());
    });
    connect(&e, &PlayEngine::endChanged, p, [=] () {
        taskbar.progress()->setRange(e.begin(), e.end());
    });
#endif
    connect(&e, &PlayEngine::tick, p, [=] (int time) {
        if (ab.check(time)) e.seek(ab.a());
#ifdef Q_OS_WIN
        taskbar.progress()->setValue(time);
#endif
    });
    connect(&e, &PlayEngine::beginSyncMrlState, p, [=] () { noMessage = true; });
    connect(&e, &PlayEngine::endSyncMrlState, p, [=] () {
        noMessage = false; const auto recorded = e.params()->resume_position() > 0;
        menu(u"play"_q)(u"seek"_q)[u"record"_q]->setEnabled(recorded);
    });
    connect(&e, &PlayEngine::started, p, [=] (const Mrl &mrl) {
        setOpen(mrl);
        if (encoder && !encoder->isBusy())
            encoder->hide();
    });
    connect(&e, &PlayEngine::finished, p, [=] (const Mrl &/*mrl*/, bool eof) {
        if (!eof) return;
        const auto next = playlist.checkNextMrl();
        if (!next.isEmpty()) load(next, true, !pref.resume_ignore_in_playlist());
    });

    connect(e.media(), &MediaObject::nameChanged, p, [=] () { updateTitle(); });

    connect(e.video()->output(), &VideoFormatObject::sizeChanged, p, [=] (const QSize &s)
        { if (pref.fit_to_video() && !s.isEmpty()) setVideoSize(s); });
    connect(&e, &PlayEngine::videoScreenRectChanged, [=] (const QRectF &r) {
        if (r.isEmpty())
            return;
        const auto num = [] (qreal n) { return _N(qRound(n)); };
        const auto w = num(r.width()), h = num(r.height());
        const auto forced = pref.osd_theme().message.show_on_resized;
        showMessage(w % u'×'_q % h, &forced);
    });
    connect(&e, &PlayEngine::volumeChanged,
            p, [=] () { if (pref.auto_unmute()) e.setAudioMuted(false); });
}

auto MainWindow::Data::initItems() -> void
{
    recent.setUpdateFunc([=] (auto &list) { this->updateRecentActions(list); });
    connect(&history, &HistoryModel::playRequested,
            p, [this] (const Mrl &mrl) { openMrl(mrl); });
    connect(&playlist, &PlaylistModel::playRequested,
            p, [this] (int row) { openMrl(playlist.at(row)); });

    hider.setSingleShot(true);
    connect(&hider, &QTimer::timeout, p, [this] () { setCursorVisible(false); });

    waiter.setInterval(500);
    waiter.setSingleShot(true);
    connect(&waiter, &QTimer::timeout, p, [=] () { updateWaitingMessage(); });

    singleClick.timer.setSingleShot(true);
    connect(&singleClick.timer, &QTimer::timeout, p,
            [=] () { if (singleClick.action) trigger(singleClick.action); });

    mouse = WindowObject::getMouse();
    connect(mouse, &MouseObject::hidingCursorBlockedChanged, p, [=] (bool b) {
        if (b) {
            hider.stop();
            setCursorVisible(true);
        } else {
            if (hidingCursorPended)
                readyToHideCursor();
            else
                cancelToHideCursor();
        }
    });

#ifdef Q_OS_WIN
    dialogWorkaround.setSingleShot(true);
    dialogWorkaround.setInterval(1);
    connect(&dialogWorkaround, &QTimer::timeout, p, [=] () {
        if (adapter && adapter->isAlwaysOnTop()) {
            adapter->setAlwaysOnTop(false);
            adapter->setAlwaysOnTop(true);
        }
    });
#endif
}

auto MainWindow::Data::updateWaitingMessage() -> void
{
    QString message;
    if (e.isWaiting())
        message = tr("%1 ...\nPlease wait for a while.").arg(e.waitingText());
    showMessageBox(message);
}

auto MainWindow::Data::openWith(const OpenMediaInfo &mode,
                                const QList<Mrl> &mrls, const QString &sub) -> void
{
    if (mrls.isEmpty())
        return;
    const auto mrl = mrls.first();
    auto checkAndPlay = [&] (const Mrl &mrl) {
        if (mrl != e.mrl())
            return false;
        if (!e.isPlaying())
            load(mrl, true, true, sub);
        return true;
    };
    if (!checkAndPlay(mrl)) {
        Playlist pl;
        if (mrl.isCueTrack())
            pl.load(mrl.cueSheet());

        switch (mode.behavior) {
        case OpenMediaBehavior::Append:
            pl.append(mrls);
            break;
        case OpenMediaBehavior::ClearAndAppend:
            playlist.clear();
            pl.append(mrls);
            break;
        case OpenMediaBehavior::NewPlaylist:
            playlist.clear();
            pl += generatePlaylist(mrl);
            break;
        }
        auto list = playlist.list();
        for (auto mrl : pl) {
            if (!list.contains(mrl))
                list.append(mrl);
        }
        playlist.setList(list);
        load(mrl, mode.start_playback, true, sub);
        if (!mrl.isDvd())
            recent.stack(mrl);
    }
    if (!p->isVisible())
        p->show();
}

auto MainWindow::Data::setVideoSize(const QSize &video) -> void
{
    if (p->isFullScreen() || p->adapter()->state() == Qt::WindowMaximized)
        return;
    if (p->rootObject()->width() != p->width())
        p->rootObject()->setWidth(p->width());
    if (p->rootObject()->height() != p->height())
        p->rootObject()->setHeight(p->height());
    // patched by Handrake
    const QSizeF vs(e.screen()->width(), e.screen()->height());
    const QSize size = (p->size() - vs.toSize() + video);
    if (size == p->size())
        return;
    const auto s = p->screen()->availableGeometry().adjusted(0, 0, 1, 1);
    const auto g = p->screen()->geometry().adjusted(0, 0, 1, 1);
    const auto m = adapter->frameMargins();
    auto pos = p->position();
    const auto prev = pos + QPoint(p->width() + m.right(), p->height() + m.bottom());
    const auto br = pos + QPoint(size.width() + m.right(), size.height() + m.bottom());
    p->resize(size);
    auto in = [] (int min, int v, int max) { return min < v && v < max; };
    if (prev.x() == s.right() || in(prev.x(), s.right(), br.x()))
        pos.rx() = qMax(s.left(), s.right() - m.right() - size.width());
    else if (prev.x() == g.right() || in(prev.x(), g.right(), br.x()))
        pos.rx() = qMax(g.left(), g.right() - m.right() - size.width());
    if (prev.y() == s.bottom() || in(prev.y(), s.bottom(), br.y()))
        pos.ry() = qMax(s.top(), s.bottom() - m.bottom() - size.height());
    else if (prev.y() == g.bottom() || in(prev.y(), g.bottom(), br.y()))
        pos.ry() = qMax(g.top(), g.bottom() - m.bottom() - size.height());
    if (pos != p->position())
        p->setPosition(pos);
}

auto MainWindow::Data::trigger(QAction *action) -> void
{
    if (!action)
        return;
    if (top->isVisible()) {
        if (unblockedActions.isEmpty()) {
            // allow only next actions when top level item shown
            unblockedActions += menu(u"window"_q).actions();
            tmp::sort(unblockedActions);
        }
        if (!tmp::contains_binary(unblockedActions, action))
            return;
    }
    action->trigger();
}

auto MainWindow::Data::setCursorVisible(bool visible) -> void
{
    if (visible && p->cursor().shape() == Qt::BlankCursor)
        p->unsetCursor();
    else if (!visible && p->cursor().shape() != Qt::BlankCursor)
        p->setCursor(Qt::BlankCursor);
    mouse->updateCursor(p->cursor().shape());
    emit p->cursorChanged(p->cursor());
}

auto MainWindow::Data::readyToHideCursor() -> void
{
    if (pref.hide_cursor()
            && (p->isFullScreen() || !pref.hide_cursor_fs_only())) {
        if (mouse->isHidingCursorBlocked())
            hidingCursorPended = true;
        else
            hider.start(pref.hide_cursor_delay_sec() * 1000);
    } else
        cancelToHideCursor();
}

auto MainWindow::Data::cancelToHideCursor() -> void
{
    hidingCursorPended = false;
    hider.stop();
    setCursorVisible(true);
}

auto MainWindow::Data::commitData() -> void
{
    static bool first = true;
    if (first) {
        recent.setLastPlaylist(playlist.list());
        recent.setLastMrl(e.mrl());
        e.shutdown();
        as.updateWindowGeometry(p);
        as.playlist_visible = playlist.isVisible();
        as.playlist_shuffled = playlist.isShuffled();
        as.playlist_repeat = playlist.repeat();
        as.history_visible = history.isVisible();
        as.state.copyFrom(e.params());
        as.save();
        e.waitUntilTerminated();
        cApp.processEvents();
        first = false;
    }
}

auto MainWindow::Data::showTimeLine() -> void
{
    if (player && pref.osd_theme().timeline.show_on_seeking)
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

auto MainWindow::Data::screenSize() const -> QSize
{
    return p->screen()->availableVirtualSize();
}

auto MainWindow::Data::openDir(const QString &dir) -> void
{
    auto dlg = dialog<OpenMediaFolderDialog>();
    dlg->setFolder(dir);
    if (dlg->exec()) {
        const auto list = dlg->playlist();
        if (!list.isEmpty()) {
            playlist.setList(list);
            load(list.first());
            recent.stack(list.first());
        }
    }
}

auto MainWindow::Data::openMrl(const Mrl &mrl) -> void
{
    if (mrl == e.mrl()) {
        if (!e.isRunning())
            load(mrl);
    } else {
        if (playlist.rowOf(mrl) < 0)
            playlist.setList(generatePlaylist(mrl));
        load(mrl);
        if (!mrl.isDvd())
            recent.stack(mrl);
    }
}

auto MainWindow::Data::openMimeData(const QMimeData *md) -> void
{
    auto urls = md->urls();
    if (md->hasText()) {
        const auto lines = md->text().split(QRegEx(uR"([\n\r]+)"_q), QString::SkipEmptyParts);
        for (auto &line : lines) {
            QUrl url(line.trimmed());
            if (!url.scheme().isEmpty() && url.isValid() && !urls.contains(url))
                urls.push_back(url);
        }
    }
    Playlist playlist;
    QStringList subList;

    auto addPlaylist = [&] (const QUrl &url, const QString &suffix) -> bool {
        if (_IsSuffixOf(PlaylistExt, suffix)) {
            Playlist list;
            list.load(url);
            playlist += list;
            return true;
        }
        const auto u = url.toString();
        if (!_IsSuffixOf(VideoExt, suffix) && !_IsSuffixOf(AudioExt, suffix)
                && !yle.supports(u) && !youtube.supports(u))
            return false;
        playlist.append(url);
        return true;
    };

    for (auto &url : urls) {
        if (url.isLocalFile()) {
            const QFileInfo fileInfo(url.toLocalFile());
            if (!fileInfo.exists())
                continue;
            auto path = fileInfo.absoluteFilePath();
            if (fileInfo.isFile()) {
                const auto suffix = fileInfo.suffix();
                if (!addPlaylist(url, suffix) && _IsSuffixOf(SubtitleExt, suffix))
                    subList << path;
            } else if (fileInfo.isDir()) {
                if (!fileInfo.fileName().compare("VIDEO_TS"_a, Qt::CaseInsensitive)
                        && !QDir(path).entryList(QStringList(u"*.ifo"_q), QDir::Files).isEmpty()) {
                    as.dvd_device = path;
                    openMrl(Mrl::fromDisc(u"dvdnav"_q, as.dvd_device, -1, true));
                } else
                    openDir(path);
                return;
            }
        } else
            addPlaylist(url, QFileInfo(url.path()).suffix().toLower());
    }
    if (!playlist.isEmpty()) {
        openWith(pref.open_media_by_drag_and_drop(), playlist);
    } else if (!subList.isEmpty())
        e.addSubtitleFiles(subList, pref.sub_enc());
}

auto MainWindow::Data::generatePlaylist(const Mrl &mrl) const -> Playlist
{
    if (mrl.isCueTrack())
        { Playlist list;  list.load(mrl.cueSheet()); return list; }
    if (!mrl.isLocalFile() || !pref.enable_generate_playlist())
        return Playlist(mrl);

    Playlist list;
    const auto mode = pref.generate_playlist();
    const QFileInfo file(mrl.toLocalFile());
    const QDir dir = file.dir();
    const auto filter = _ToNameFilter(pref.exclude_images() ? VideoExt | AudioExt : MediaExt);
    if (mode == GeneratePlaylist::Folder) {
        const auto files = dir.entryList(filter, QDir::Files, QDir::Name);
        for (int i=0; i<files.size(); ++i)
            list.push_back(dir.absoluteFilePath(files[i]));
    } else {
        const auto files = dir.entryInfoList(filter, QDir::Files, QDir::Name);
        const auto fileName = file.fileName();
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
    }

    if (list.size()) {
        list.sort();
        return list;
    } else {
        return Playlist(mrl);
    }
}

auto MainWindow::Data::showMessage(const QString &msg, const bool *force) -> void
{
    if (noMessage)
        return;
    if (force) {
        if (!*force)
            return;
    } else if (!pref.osd_theme().message.show_on_action)
        return;
    if (p->m_sgInit)
        showOSD(msg);
}

auto MainWindow::Data::applyPref() -> void
{
    pref.save();
    const Pref &p = pref;

    OS::setScreensaverMethod(p.screensaver_method());
    const auto acc = p.sub_enc_autodetection() ? p.sub_enc_accuracy() * 1e-2 : -1;
    EncodingInfo::setDefault(EncodingInfo::Subtitle, p.sub_enc(), acc);

    const auto &controls = p.controls_theme();

    e.preview()->setShowKeyframe(controls.showKeyframeForPreview);
    youtube.setUserAgent(p.yt_user_agent());
    youtube.setProgram(p.yt_program());
    youtube.setPreferredFormat(p.yt_height(), p.yt_fps(), p.yt_container());
    yle.setProgram(p.yle_program());
    history.setRememberImage(p.remember_image());
    history.setPropertiesToRestore(p.restore_properties());
    history.setShowMediaTitleInName(controls.showMediaTitleForLocalFilesInHistory,
                                    controls.showMediaTitleForUrlsInHistory);
    if (subFindDlg)
        subFindDlg->setOptions(pref.preserve_downloaded_subtitles(),
                               pref.preserve_file_name_format(),
                               pref.preserve_fallback_folder());
    SubtitleParser::setMsPerCharactor(p.ms_per_char());
    cApp.setMprisActivated(p.use_mpris2());

    if (p.jr_use()) {
        _Renew(jrServer, p.jr_connection(), p.jr_protocol());
        jrServer->setInterface(&jrPlayer);
        jrServer->setErrorHandler([=] (auto) {
            MBox::error(nullptr, tr("JSON-RPC Server Error"),
                        jrServer->errorString(), {BBox::Ok});
        });
        jrServer->listen(p.jr_address(), p.jr_port());
    } else
        _Delete(jrServer);

    MouseBehavior context = MouseBehavior::NoBehavior;
    contextMenuModifier = KeyModifier::None;
    const auto map = p.mouse_action_map();
    for (auto it = map.begin(); it != map.end(); ++it) {
        for (auto iit = it->begin(); iit != it->end(); ++iit) {
            if (iit.value() != "context-menu"_a)
                continue;
            context = it.key();
            contextMenuModifier = iit.key();
            break;
        }
        if (context != MouseBehavior::NoBehavior)
            break;
    }
    if (context == MouseBehavior::NoBehavior) {
        _Warn("No mouse behavior bound for context menu. Enforce right click.");
        contextMenuButton = Qt::RightButton;
    } else {
        int button = _EnumData(context);
        if (button < 0)
            button = Qt::NoButton;
        contextMenuButton = static_cast<Qt::MouseButton>(button);
    }

    menu.retranslate();
    menu.setShortcutMap(p.shortcut_map());
    auto &play = menu(u"play"_q);
    play(u"speed"_q).s()->setValue(p.steps().speed_pct);
    auto &seek = play(u"seek"_q);
    seek.s(u"seek1"_q)->setValue(p.steps().seek1_sec);
    seek.s(u"seek2"_q)->setValue(p.steps().seek2_sec);
    seek.s(u"seek3"_q)->setValue(p.steps().seek3_sec);
    auto &video = menu(u"video"_q);
    video(u"aspect"_q).s()->setValue(p.steps().aspect_ratio);
    video(u"zoom"_q).s()->setValue(p.steps().zoom_pct);
    video(u"move"_q).s(u"horizontal"_q)->setValue(p.steps().video_offset_pct);
    video(u"move"_q).s(u"vertical"_q)->setValue(p.steps().video_offset_pct);
    auto &color = video(u"color"_q);
    color.s(u"brightness"_q)->setValue(p.steps().color_pct);
    color.s(u"contrast"_q)->setValue(p.steps().color_pct);
    color.s(u"hue"_q)->setValue(p.steps().color_pct);
    color.s(u"saturation"_q)->setValue(p.steps().color_pct);
    color.s(u"red"_q)->setValue(p.steps().color_pct);
    color.s(u"green"_q)->setValue(p.steps().color_pct);
    color.s(u"blue"_q)->setValue(p.steps().color_pct);
    auto &audio = menu(u"audio"_q);
    audio(u"sync"_q).s()->setValue(p.steps().audio_sync_sec);
    audio(u"volume"_q).s()->setValue(p.steps().volume_pct);
    audio(u"amp"_q).s()->setValue(p.steps().amp_pct);
    auto &sub = menu(u"subtitle"_q);
    sub(u"position"_q).s()->setValue(p.steps().sub_pos_pct);
    sub(u"sync"_q).s()->setValue(p.steps().sub_sync_sec);
    sub(u"scale"_q).s()->setValue(p.steps().sub_scale_pct);

    auto &win = menu(u"window"_q);
    for (int i = 0; i < p.window_sizes().size(); ++i)
        p.window_sizes()[i].fillAction(win["size"_a % _N(i)]);

    theme.set(p.osd_theme());
    theme.set(controls);
    reloadSkin();
    if (tray)
        tray->setVisible(p.enable_system_tray());

    auto cache = [&] () {
        CacheInfo cache;
        cache.local.kb = p.cache_local_mb() * 1024.0;
        cache.network.kb = p.cache_network_mb() * 1024.0;
        cache.disc.kb = p.cache_disc_mb() * 1024.0;
        cache.local.sec = p.cache_local_sec();
        cache.network.sec = p.cache_network_sec();
        cache.disc.sec = p.cache_disc_sec();
        cache.local.file = p.cache_local_file();
        cache.network.file = p.cache_network_file();
        cache.disc.file = p.cache_disc_file();
        cache.file_kb = p.cache_file_size_mb() * 1024.0;
        cache.min_playback_kb = p.cache_min_playback_kb();
        cache.min_seeking_kb = p.cache_min_seeking_kb();
        cache.remotes = p.network_folders();
        return cache;
    };
    auto smb = [&] () {
        SmbAuth smb;
        smb.setUsername(p.smb_username());
        smb.setPassword(p.smb_password());
        smb.setGetAuthInfo([=] (SmbAuth *smb) -> bool {
            QMutex mutex; QWaitCondition cond;
            bool res = false;
            mutex.lock();
            _PostEvent(this->p, GetSmbAuth, smb, &res, &cond);
            cond.wait(&mutex);
            mutex.unlock();
            return res;
        });
        return smb;
    };

    e.lock();
    e.preview()->setActive(controls.showPreviewOnMouseOverSeekBar);

    e.setResume_locked(p.remember_stopped());
    e.setPreciseSeeking_locked(p.precise_seeking());
    e.setCache_locked(cache());
    e.setSmbAuth_locked(smb());
    e.setPriority_locked(p.audio_priority(), p.sub_priority());
    e.setAutoloader_locked(p.audio_autoload(), p.sub_autoload_v2());

    e.setHwAcc_locked(p.enable_hwaccel(), p.hwaccel_codecs());
    e.setDeintOptions_locked(p.deinterlacing());
    e.setMotionIntrplOption_locked(p.motion_interpolation());

    e.setAudioDevice_locked(p.audio_device());
    e.setVolumeNormalizerOption_locked(p.audio_normalizer());
    e.setChannelLayoutMap_locked(p.channel_manipulation());
    e.setVolumeControl_locked(p.volume_scale(), p.soft_clip());
    e.setResyncAvWhenFilterToggled_locked(p.audio_filter_resync());

    e.setSubtitleStyle_locked(p.sub_style());
    e.setAutoselectMode_locked(p.sub_enable_autoselect(), p.sub_autoselect(),
                               p.sub_ext(), p.sub_prefer_external());
    e.unlock();
    e.reload();
}

auto MainWindow::Data::updateStaysOnTop() -> void
{
    if (p->adapter()->state() & Qt::WindowMinimized)
        return;
    const auto id = as.win_stays_on_top;
    bool onTop = !p->isFullScreen();
    onTop &= (id == StaysOnTop::Always)
          || (id == StaysOnTop::Playing && e.isPlaying());
    adapter->setAlwaysOnTop(onTop);
}

auto MainWindow::Data::videoSize(const WindowSize &hint) -> QSize
{
    auto area = [] (const QSizeF &s) { return s.width()*s.height(); };
    double r = hint.rate;
    const QSizeF video = e.videoSizeHint();
    if (hint.display_based)
        r = area(screenSize()) * hint.rate / area(video);
    return e.renderSizeHint((video * qSqrt(r)).toSize());
}

auto MainWindow::Data::load(const Mrl &mrl, bool play, bool tryResume,
                            const QString &sub) -> void
{
    if (play)
        e.load(mrl, tryResume, sub);
    else
        e.setMrl(mrl);
}

auto MainWindow::Data::clear() -> void
{
    this->player = nullptr;
    p->setSource(QUrl());
    p->qmlEngine()->clearComponentCache();
    p->releaseResources();
}

auto MainWindow::Data::reloadSkin() -> void
{
    clear();
    Skin::apply(p, pref.skin_name());
    if (p->status() == QQuickView::Error) {
        QString msg;
        for (auto &error : p->errors())
            msg += error.toString() % '\n'_q;
        MBox::error(nullptr, tr("Error on loading skin"), msg, {BBox::Ok});
        return;
    }
    auto app = p->rootObject();
    if (!app)
        return;
    auto min = app->property("minimumSize").toSize();
    if (min.width() < 300)
        min.rwidth() = 300;
    if (min.height() < 200)
        min.rheight() = 200;
    p->setMinimumSize(min);
    if (p->width() < min.width() || p->height() < min.height())
        p->resize(min);
    QQmlProperty pplayer(app, u"player"_q);
    if (pplayer.isValid()) {
        pplayer.connectNotifySignal(p, SLOT(setupSkinPlayer()));
        p->setupSkinPlayer();
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
}

auto MainWindow::Data::updateMrl(const Mrl &mrl) -> void
{
    const auto disc = mrl.isDisc();
    playlist.setLoaded(mrl);
    auto action = menu(u"play"_q)[u"disc-menu"_q];
    action->setEnabled(disc);
    action->setVisible(disc);

    filePath.clear();
    if (mrl.isLocalFile()) {
        const QFileInfo file(mrl.toLocalFile());
        filePath = file.absoluteFilePath();
        if (p->isVisible())
            p->setFilePath(filePath);
    } else
        p->setFilePath(QString());
}

auto MainWindow::Data::updateTitle() -> void
{
    cApp.setWindowTitle(p, e.media()->name());
}

auto MainWindow::Data::doVisibleAction(bool visible) -> void
{
    if (visible) {
        if (pausedByHiding) {
            e.unpause();
            pausedByHiding = false;
        }
        p->setFilePath(filePath);
#ifndef Q_OS_MAC
        p->setIcon(cApp.defaultIcon());
#endif
        e.updateVideoGeometry();
    } else {
        if (!pref.pause_minimized() || !e.isPlaying())
            return;
        if (pref.pause_video_only() && e.isAudioOnly())
            return;
        pausedByHiding = true;
        e.pause();
    }
}

auto MainWindow::Data::updateWindowState(Qt::WindowState ws) -> void
{
    p->setFilePath(filePath);
    adapter->endMoveByDrag();
    readyToHideCursor();
    auto changed = [&] (Qt::WindowState state) -> bool
        { return !(prevWindowState & state) != !(ws & state); };
    if (!stateChanging && changed(Qt::WindowMinimized))
        doVisibleAction(ws != Qt::WindowMinimized);
    const bool fsChanged = changed(Qt::WindowFullScreen);
    if (fsChanged)
        emit p->fullscreenChanged(p->isFullScreen());
    updateStaysOnTop();
    if (fsChanged && !p->isFullScreen())
        as.restoreLastWindowGeometry(p);
    prevWindowState = ws;
}

auto MainWindow::Data::fileNameGenerator(const QTime &end) const -> FileNameGenerator
{
    FileNameGenerator g;
    g.dateTime = QDateTime::currentDateTime();
    g.start = QTime::fromMSecsSinceStartOfDay(e.time());
    g.end = end.isNull() ? g.start : end;
    g.unix_ = QDateTime::currentMSecsSinceEpoch();
    g.mrl = e.mrl();
    g.mediaName = e.media()->name();
    return g;
}
