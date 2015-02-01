#include "mainwindow_p.hpp"
#include "app.hpp"
#include "enum/movetoward.hpp"
#include "subtitle/subtitleview.hpp"
#include "dialog/mbox.hpp"
#include "dialog/audioequalizerdialog.hpp"
#include "dialog/urldialog.hpp"
#include "dialog/prefdialog.hpp"
#include "dialog/aboutdialog.hpp"
#include "dialog/opendiscdialog.hpp"
#include "dialog/snapshotdialog.hpp"
#include "dialog/subtitlefinddialog.hpp"
#include "dialog/encodingfiledialog.hpp"

auto MainWindow::Data::triggerNextAction(const QList<QAction*> &actions) -> void
{
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
}

auto MainWindow::Data::plugTrack(Menu &parent, void(MrlState::*sig)(StreamList),
                                 void(PlayEngine::*set)(int,bool),
                                 const QString &gkey, QAction *sep) -> void
{
    auto *m = &parent(u"track"_q);
    const auto g = m->g(gkey);
    connect(e.params(), sig, p, [=] (StreamList list) {
        g->clear();
        if (!list.isEmpty()) {
            for (auto it = list.begin(); it != list.end(); ++it) {
                auto act = new QAction(it->name(), m);
                m->insertAction(sep, act);
                g->addAction(act);
                act->setCheckable(true);
                act->setData(it->id());
                act->setChecked(it->isSelected());
            }
        }
    });
    connect(g, &ActionGroup::triggered, p, [=] (QAction *a)
        { (e.*set)(a->data().toInt(), a->isChecked()); });
}

template<class F>
auto MainWindow::Data::plugStreamActions(Menu *menu, F func,
                                         const QString &group) -> void
{
    auto checkCurrentStreamAction = [=] () {
        const auto current = (e.*func)();
        for (auto action : menu->g(group)->actions()) {
            if (action->data().toInt() == current) {
                action->setChecked(true);
                break;
            }
        }
    };
    connect(menu, &QMenu::aboutToShow, p, checkCurrentStreamAction);
}

auto MainWindow::Data::connectMenus() -> void
{
    Menu &open = menu(u"open"_q);
    connect(open[u"file"_q], &QAction::triggered, p, [this] () {
        const auto file = _GetOpenFile(p, tr("Open File"), MediaExt);
        if (!file.isEmpty())
            openMrl(Mrl(file));
    });
    connect(open[u"folder"_q], &QAction::triggered, p, [this] () { openDir(); });
    connect(open[u"url"_q], &QAction::triggered, p, [this] () {
        UrlDialog dlg(p);
        if (dlg.exec()) {
            if (dlg.isPlaylist())
                playlist.open(dlg.url(), dlg.encoding());
            else
                openMrl(dlg.url().toString());
        }
    });
    auto openDisc = [this] (const QString &title, QString &device, bool dvd) {
        OpenDiscDialog dlg(p);
        dlg.setIsoEnabled(dvd);
        dlg.setWindowTitle(title);
        dlg.setDeviceList(cApp.devices());
        if (!device.isEmpty())
            dlg.setDevice(device);
        if (dlg.exec())
            device = dlg.device();
        return dlg.result() && !device.isEmpty();
    };

    connect(open[u"dvd"_q], &QAction::triggered, p, [openDisc, this] () {
        if (openDisc(tr("Select DVD device"), as.dvd_device, true))
            openMrl(Mrl::fromDisc(u"dvdnav"_q, as.dvd_device, -1, true));
    });
    connect(open[u"bluray"_q], &QAction::triggered, p, [openDisc, this] () {
        if (openDisc(tr("Select Blu-ray device"), as.bluray_device, false))
            openMrl(Mrl::fromDisc(u"bdnav"_q, as.bluray_device, -1, true));
    });
    connect(open(u"recent"_q).g(), &ActionGroup::triggered,
            p, [this] (QAction *a) {openMrl(Mrl(a->data().toString()));});
    connect(open(u"recent"_q)[u"clear"_q], &QAction::triggered,
            &recent, &RecentInfo::clear);

    Menu &play = menu(u"play"_q);
    connect(play[u"stop"_q], &QAction::triggered,
            p, [this] () {e.stop();});
    PLUG_STEP(play(u"speed"_q).g(), play_speed, setSpeedPercent);

    connect(play[u"pause"_q], &QAction::triggered, p, &MainWindow::togglePlayPause);
    connect(play(u"repeat"_q).g(), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        const int key = a->data().toInt();
        auto msg = [=] (const QString &s) { showMessage(tr("A-B Repeat"), s); };
        auto time = [] (int t) {
            const auto time = QTime::fromMSecsSinceStartOfDay(t);
            auto str = time.toString(u"h:mm:ss.zzz"_q);
            str.chop(2); return str;
        };
        switch (key) {
        case 'r': {
            if (e.isStopped())
                break;
            const auto t = e.time();
            if (!ab.hasA()) {
                msg(tr("Set A to %1").arg(time(ab.setA(t))));
            } else if (!ab.hasB()) {
                const int at = ab.setB(t);
                if ((at - ab.a()) < 100) {
                    ab.setB(-1);
                    msg(tr("Range is too short!"));
                } else {
                    ab.start();
                    msg(tr("Set B to %1. Start to repeat!").arg(time(at)));
                }
            }
            break;
        } case 's': {
            ab.setA(e.captionBeginTime());
            ab.setB(e.captionEndTime());
            ab.start();
            msg(tr("Repeat current subtitle"));
            break;
        } case 'q':
            ab.stop();
            ab.setA(-1);
            ab.setB(-1);
            msg(tr("Quit repeating"));
        }
    });
    connect(play[u"prev"_q], &QAction::triggered,
            &playlist, &PlaylistModel::playPrevious);
    connect(play[u"next"_q], &QAction::triggered,
            &playlist, &PlaylistModel::playNext);
    auto &seek = play(u"seek"_q);
    connect(seek[u"begin"_q], &QAction::triggered, p, [=] () {
        e.seek(e.begin());
    });
    connect(seek.g(u"relative"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        const int diff = static_cast<StepAction*>(a)->data();
        if (diff && !e.isStopped() && e.isSeekable()) {
            e.relativeSeek(diff);
            showMessage(tr("Seeking"), diff/1000, tr("sec"), true);
            showTimeLine();
        }
    });
    connect(seek.g(u"frame"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        e.stepFrame(a->data().toInt());
    });
    connect(seek[u"black-frame"_q], &QAction::triggered, p, [=] () {
        e.seekToNextBlackFrame();
        showMessage(tr("Seek to Next Black Frame"));
    });
    connect(play[u"disc-menu"_q], &QAction::triggered, p, [this] () {
        e.setCurrentEdition(PlayEngine::DVDMenu);
    });
    connect(seek.g(u"subtitle"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) { e.seekCaption(a->data().toInt()); });
    connect(play(u"title"_q).g(), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        a->setChecked(true);
        e.setCurrentEdition(a->data().toInt());
        showMessage(tr("Current Title"), a->text());
    });
    connect(play(u"chapter"_q).g(), &ActionGroup::triggered, p,
            [this] (QAction *a) {
        a->setChecked(true);
        e.setCurrentChapter(a->data().toInt());
        showMessage(tr("Current Chapter"), a->text());
    });
    auto seekChapter = [this] (int offset) {
        if (!e.chapters().isEmpty()) {
            auto target = e.currentChapter() + offset;
            if (target > -2)
                e.setCurrentChapter(target);
        }
    };
    connect(play(u"chapter"_q)[u"prev"_q], &QAction::triggered,
            p, [seekChapter] () { seekChapter(-1); });
    connect(play(u"chapter"_q)[u"next"_q], &QAction::triggered, p,
            [seekChapter] () { seekChapter(+1); });

    Menu &video = menu(u"video"_q);

    PLUG_ENUM(video(u"aspect"_q), video_aspect_ratio, setVideoAspectRatio);
    PLUG_ENUM(video(u"crop"_q), video_crop_ratio, setVideoCropRatio);

    auto &snap = video(u"snapshot"_q);
    auto connectSnapshot = [&] (const QString &actionName, SnapshotMode mode) {
        connect(snap[actionName], &QAction::triggered, p, [this, mode] () {
            snapshotMode = mode;
            if (snapshotMode == NoSnapshot)
                return;
            PlayEngine::Snapshot snapshot = PlayEngine::VideoOnly;
            if (snapshotMode == QuickSnapshot || snapshotMode == SnapshotTool)
                snapshot = PlayEngine::VideoAndOsd;
            e.takeSnapshot(snapshot);
        });
    };
    connectSnapshot(u"quick"_q, QuickSnapshot);
    connectSnapshot(u"quick-nosub"_q, QuickSnapshotNoSub);
    connectSnapshot(u"tool"_q, SnapshotTool);
    connect(&e, &PlayEngine::snapshotTaken, p, [this] () {
        auto video = e.snapshot(false);
        auto osd = e.snapshot(true);
        e.clearSnapshots();
        if (video.isNull() && osd.isNull())
            return;
        if (snapshotMode == QuickSnapshot || snapshotMode == SnapshotTool) {
            QRectF subRect;
            auto sub = e.subtitleImage(osd.rect(), &subRect);
            if (!sub.isNull()) {
                QPainter painter(&osd);
                painter.drawImage(subRect, sub);
            }
        }
        switch (snapshotMode) {
        case SnapshotTool: {
            if (!snapshot) {
                snapshot = new SnapshotDialog(p);
                connect(snapshot, &SnapshotDialog::request, p, [=] () {
                    if (e.hasVideoFrame()) {
                        snapshotMode = SnapshotTool;
                        e.takeSnapshot(PlayEngine::VideoAndOsd);
                    } else
                        snapshot->clear();
                });
            }
            if (!video.isNull())
                snapshot->setImage(video, osd);
            else
                snapshot->clear();
            break;
        } case QuickSnapshot: case QuickSnapshotNoSub: {
            auto image = video;
            if (snapshotMode == QuickSnapshot)
                image = osd;
            const auto time = QDateTime::currentDateTime();
            const QString fileName = "bomi-snapshot-"_a
                    % time.toString(u"yyyy-MM-dd-hh-mm-ss-zzz"_q)
                    % '.'_q % pref().quick_snapshot_format;
            QString file;
            switch (pref().quick_snapshot_save) {
            case QuickSnapshotSave::Current:
                if (e.mrl().isLocalFile()) {
                    file = _ToAbsPath(e.mrl().toLocalFile())
                           % '/'_q % fileName;
                    break;
                }
            case QuickSnapshotSave::Ask:
                file = _GetSaveFile(p, tr("Save File"), fileName, WritableImageExt);
                break;
            case QuickSnapshotSave::Fixed:
                file = pref().quick_snapshot_folder % '/'_q % fileName;
                break;
            }
            if (!file.isEmpty() && image.save(file, nullptr,
                                              pref().quick_snapshot_quality))
                showMessage(tr("Snapshot saved"), fileName);
            else
                showMessage(tr("Failed to save a snapshot"));
            break;
        } default:
            break;
        }
    }, Qt::QueuedConnection);

    PLUG_ENUM(video(u"align"_q), video_vertical_alignment, setVideoVerticalAlignment);
    PLUG_ENUM(video(u"align"_q), video_horizontal_alignment, setVideoHorizontalAlignment);

    connect(&video(u"move"_q), &Menu::triggered, p, [=] (QAction *a) {
        const auto diff = static_cast<EnumAction<MoveToward>*>(a)->data();
        if (diff.isNull())
            e.setVideoOffset(diff);
        else
            e.setVideoOffset(e.params()->video_offset() + diff);
    });

    PLUG_ENUM_CHILD(video, video_deinterlacing, setDeintMode);
    PLUG_ENUM(video(u"interpolator"_q), video_interpolator, setInterpolator);
    PLUG_ENUM(video(u"chroma-upscaler"_q), video_chroma_upscaler, setChromaUpscaler);

    PLUG_FLAG(video(u"hq-scaling"_q)[u"down"_q], video_hq_downscaling, setVideoHighQualityDownscaling);
    PLUG_FLAG(video(u"hq-scaling"_q)[u"up"_q], video_hq_upscaling, setVideoHighQualityUpscaling);

    PLUG_ENUM_CHILD(video, video_dithering, setVideoDithering);
    PLUG_ENUM_CHILD(video, video_space, setColorSpace);
    PLUG_ENUM_CHILD(video, video_range, setColorRange);

    connect(&video(u"filter"_q), &Menu::triggered, p, [this] () {
        VideoEffects effects = 0;
        for (auto act : menu(u"video"_q)(u"filter"_q).actions()) {
            if (act->isChecked())
                effects |= act->data().value<VideoEffect>();
        }
        push(effects, "video_effects", &MrlState::video_effects,
             &PlayEngine::setVideoEffects, _EnumFlagsDescription<VideoEffect>);
    });
    connect(e.params(), &MrlState::video_effects_changed, &e,
            [=] (VideoEffects effects) {
        for (auto a : menu(u"video"_q)(u"filter"_q).actions())
            a->setChecked(a->data().value<VideoEffect>() & effects);
    });
    VideoColor::for_type([=, &video] (VideoColor::Type type) {
        const auto name = VideoColor::name(type);
        connect(video(u"color"_q).g(name), &ActionGroup::triggered, p,
                [=] (QAction *a) {
            const int diff = static_cast<StepAction*>(a)->data();
            auto color = e.params()->video_color();
            color.add(type, diff);
            push(color, "video_color", &MrlState::video_color,
                 &PlayEngine::setVideoEqualizer, [=] (const VideoColor &eq)
                 { return eq.formatText(type).arg(eq.get(type)); });
        });
    });
    connect(video(u"color"_q)[u"reset"_q], &QAction::triggered, p, [this] () {
        const VideoColor eq, old = e.params()->video_color();
        if (old != eq)
            push(VideoColor(), e.params()->video_color(), [=] (const VideoColor &eq) {
                e.setVideoEqualizer(eq);
                showProperty("video_color", eq.isZero() ? tr("Reset") : tr("Restore"));
            });
        showProperty("video_color", tr("Reset"));
    });

    Menu &audio = menu(u"audio"_q);

    PLUG_STEP(audio(u"volume"_q).g(), audio_volume, setAudioVolume);
    PLUG_FLAG(audio(u"volume"_q)[u"mute"_q], audio_muted, setAudioMuted);
    connect(e.params(), &MrlState::audio_equalizer_changed, p, [=] (auto aeq)
        { if (eq) { QSignalBlocker bl(eq); eq->setEqualizer(aeq); } });

    connect(audio[u"equalizer"_q], &QAction::triggered, p, [=] () {
        if (!eq) {
            eq = new AudioEqualizerDialog(p);
            connect(eq, &AudioEqualizerDialog::equalizerChanged, p,
                    [=] (const AudioEqualizer &eq) { e.setAudioEqualizer(eq); });
        }
        eq->setEqualizer(e.params()->audio_equalizer());
        eq->show();
    });
    PLUG_STEP(audio(u"sync"_q).g(), audio_sync, setAudioSync);
    PLUG_STEP(audio(u"amp"_q).g(), audio_amplifier, setAudioAmpPercent);
    PLUG_ENUM_CHILD(audio, audio_channel_layout, setChannelLayout);
    PLUG_FLAG(audio[u"normalizer"_q], audio_volume_normalizer, setAudioVolumeNormalizer);
    PLUG_FLAG(audio[u"tempo-scaler"_q], audio_tempo_scaler, setAudioTempoScaler);


//    PLUG_STEP(play)

    Menu &sub = menu(u"subtitle"_q);
    auto &atrack = audio(u"track"_q);
    auto &strack = sub(u"track"_q);
    auto ssep = strack.addSeparator();
    plugTrack(video, &MrlState::video_tracks_changed, &PlayEngine::setVideoTrackSelected);
    plugTrack(audio, &MrlState::audio_tracks_changed, &PlayEngine::setAudioTrackSelected);
    plugTrack(sub, &MrlState::sub_tracks_changed,
              &PlayEngine::setSubtitleTrackSelected, "exclusive"_a);
    plugTrack(sub, &MrlState::sub_tracks_inclusive_changed,
              &PlayEngine::setSubtitleInclusiveTrackSelected, "inclusive"_a, ssep);
    connect(audio(u"track"_q)[u"next"_q], &QAction::triggered, p, [=] ()
        { triggerNextAction(menu(u"audio"_q)(u"track"_q).g()->actions()); });
    connect(sub(u"track"_q)[u"next"_q], &QAction::triggered, p, [=] () {
        auto &m = menu(u"subtitle"_q)(u"track"_q);
        auto actions = m.g(u"exclusive"_q)->actions();
        actions += m.g(u"inclusive"_q)->actions();
        triggerNextAction(actions);
    });


    connect(atrack[u"open"_q], &QAction::triggered, p, [this] () {
        const auto files = _GetOpenFiles(p, tr("Open Audio File"), AudioExt);
        if (!files.isEmpty())
            e.addAudioFiles(files);
    });
    connect(atrack[u"auto-load"_q], &QAction::triggered, &e, &PlayEngine::autoloadAudioFiles);
    connect(atrack[u"reload"_q], &QAction::triggered, &e, &PlayEngine::reloadAudioFiles);
    connect(atrack[u"clear"_q], &QAction::triggered, &e, &PlayEngine::clearAudioFiles);

    connect(&e, &PlayEngine::editionsChanged,
            p, [this] (const EditionList &editions) {
        const auto edition = e.currentEdition();
//        updateListMenu(menu(u"play"_q)(u"title"_q), editions, edition);
    });
    connect(&e, &PlayEngine::chaptersChanged,
            p, [this] (const ChapterList &chapters) {
        const auto chapter = e.currentChapter();
//        updateListMenu(menu(u"play"_q)(u"chapter"_q), chapters, chapter);
    });

    connect(strack[u"all"_q], &QAction::triggered, p, [=, &strack] () {
        e.setSubtitleInclusiveTrackSelected(-1, true);
        const auto size = e.params()->sub_tracks_inclusive().size();
        const auto text = tr("%1 Subtitle(s)").arg(size);
        showMessage(tr("Select All Subtitles"), text);
    });
    PLUG_FLAG(strack[u"hide"_q], sub_hidden, setSubtitleHidden);
    connect(strack[u"open"_q], &QAction::triggered, p, [this] () {
        QString dir;
        if (e.mrl().isLocalFile())
            dir = _ToAbsPath(e.mrl().toLocalFile());
        QString enc = pref().sub_enc;
        const auto files = EncodingFileDialog::getOpenFileNames(
            p, tr("Open Subtitle"), dir, _ToFilter(SubtitleExt), &enc);
        e.addSubtitleFiles(files, enc);
    });
    connect(strack[u"auto-load"_q], &QAction::triggered, &e, &PlayEngine::autoloadSubtitleFiles);
    connect(strack[u"reload"_q], &QAction::triggered, &e, &PlayEngine::reloadSubtitleFiles);
    connect(strack[u"clear"_q], &QAction::triggered, &e, &PlayEngine::clearSubtitleFiles);

    PLUG_ENUM_CHILD(sub, sub_display, setSubtitleDisplay);
    PLUG_ENUM(sub(u"align"_q), sub_alignment, setSubtitleAlignment);
    PLUG_STEP(sub(u"position"_q).g(), sub_position, setSubtitlePosition);
    PLUG_STEP(sub(u"sync"_q).g(), sub_sync, setSubtitleDelay);

    Menu &tool = menu(u"tool"_q);
    auto &pl = tool(u"playlist"_q);
    connect(pl[u"toggle"_q], &QAction::triggered, &playlist, &PlaylistModel::toggle);
    connect(pl[u"open"_q], &QAction::triggered, p, [this] () {
        QString enc;
        const auto filter = _ToFilter(PlaylistExt);
        const auto file = EncodingFileDialog::getOpenFileName
                (p, tr("Open File"), QString(), filter, &enc);
        if (!file.isEmpty())
            playlist.open(file, enc);
    });
    connect(pl[u"save"_q], &QAction::triggered, p, [this] () {
        const auto &list = playlist.list();
        if (!list.isEmpty()) {
            auto file = _GetSaveFile(p, tr("Save File"),
                                         QString(), PlaylistExt);
            if (!file.isEmpty())
                list.save(file);
        }
    });
    connect(pl[u"clear"_q], &QAction::triggered, p, [=] () { playlist.clear(); });
    connect(pl[u"append-file"_q], &QAction::triggered, p, [this] () {
        const auto files = _GetOpenFiles(p, tr("Open File"), MediaExt);
        Playlist list;
        for (int i=0; i<files.size(); ++i)
            list.push_back(Mrl(files[i]));
        playlist.append(list);
    });
    connect(pl[u"append-url"_q], &QAction::triggered, p, [this] () {
        UrlDialog dlg(p);
        if (dlg.exec()) {
            const Mrl mrl = dlg.url().toString();
            if (dlg.isPlaylist()) {
                Playlist list;
                list.load(mrl, dlg.encoding());
                playlist.append(list);
            } else
                playlist.append(mrl);
        }
    });
    connect(pl[u"remove"_q], &QAction::triggered, p,
            [=] () { playlist.remove(playlist.selected()); });
    connect(pl[u"move-up"_q], &QAction::triggered, p, [=] () {
        const auto idx = playlist.selected();
        if (playlist.swap(idx, idx-1))
            playlist.select(idx-1);
    });
    connect(pl[u"move-down"_q], &QAction::triggered, p, [=] () {
        const auto idx = playlist.selected();
        if (playlist.swap(idx, idx+1))
            playlist.select(idx+1);
    });
    auto action = pl[u"shuffle"_q];
    connect(action, &QAction::triggered, p, [=] (bool new_) {
        const bool old = playlist.isShuffled();
        if (new_ != old) {
            push(new_, old, [=] (bool checked) {
                playlist.setShuffled(checked);
                action->setChecked(checked);
                showMessage(tr("Shuffle Playlist"), checked);
            });
        }
    });
    action = pl[u"repeat"_q];
    connect(action, &QAction::triggered, p, [=] (bool new_) {
        const bool old = playlist.repeat();
        if (new_ != old) {
            push(new_, old, [=] (bool checked) {
                playlist.setRepeat(checked);
                action->setChecked(checked);
                showMessage(tr("Repeat Playlist"), checked);
            });
        }
    });

    auto &hm = tool(u"history"_q);
    connect(hm[u"toggle"_q], &QAction::triggered, &history, &HistoryModel::toggle);
    connect(hm[u"clear"_q], &QAction::triggered, p, [=] () { history.clear(); });

    connect(tool[u"playinfo"_q], &QAction::triggered, p, [=] () {
        auto toggleTool = [this] (const char *name, bool &visible) {
            visible = !visible;
            if (auto item = view->findItem<QObject>(_L(name)))
                item->setProperty("show", visible);
        };
        toggleTool("playinfo", as.playinfo_visible);
    });
    connect(tool[u"subtitle"_q], &QAction::triggered, p, [this] () {
        subtitleView->setVisible(!subtitleView->isVisible());
    });
    connect(tool[u"pref"_q], &QAction::triggered, p, [this] () {
        if (!prefDlg) {
            prefDlg = new PrefDialog(p);
            prefDlg->setAudioDeviceList(e.audioDeviceList());
            connect(prefDlg, &PrefDialog::applyRequested, p, [this] {
                prefDlg->get(preferences); applyPref();
            });
            connect(prefDlg, &PrefDialog::resetRequested, p, [this] {
                prefDlg->set(pref());
            });
        }
        prefDlg->set(pref());
        prefDlg->show();
    });
    connect(tool[u"find-subtitle"_q], &QAction::triggered, p, [this] () {
        if (!subFindDlg) {
            subFindDlg = new SubtitleFindDialog(p);
            subFindDlg->setSelectedLangCode(as.sub_find_lang_code);
            connect(subFindDlg, &SubtitleFindDialog::loadRequested,
                    p, [this] (const QString &fileName) {
                e.addSubtitleFiles(QStringList(fileName), pref().sub_enc);
                showMessage(tr("Downloaded"), QFileInfo(fileName).fileName());
            });
        }
        subFindDlg->find(e.mrl());
        subFindDlg->show();
    });
    connect(tool[u"reload-skin"_q], &QAction::triggered,
            p, [=] () { reloadSkin(); });
    connect(tool[u"auto-exit"_q], &QAction::triggered, p, [this] (bool on) {
        if (on != as.auto_exit)
            push(on, as.auto_exit, [this] (bool on) {
                as.auto_exit = on;
                showMessage(on ? tr("Exit bomi when the playlist has finished.")
                               : tr("Auto-exit is canceled."));
                menu(u"tool"_q)[u"auto-exit"_q]->setChecked(on);
            });
    });
    connect(tool[u"auto-shutdown"_q], &QAction::toggled, p, [this] (bool on) {
        if (on) {
            if (MBox::warn(p, tr("Auto-shutdown"),
                           tr("The system will shut down "
                              "when the play list has finished."),
                           {BBox::Ok, BBox::Cancel}) == BBox::Cancel) {
                menu(u"tool"_q)[u"auto-shutdown"_q]->setChecked(false);
            } else
                showMessage(tr("The system will shut down "
                               "when the play list has finished."));
        } else
            showMessage(tr("Auto-shutdown is canceled."));
    });

    Menu &win = menu(u"window"_q);
    plugAppEnumChild(win, "win_stays_on_top", &AppState::winStaysOnTopChanged);
    connect(&as, &AppState::winStaysOnTopChanged, p, [=] () { updateStaysOnTop(); });
    connect(win.g(u"size"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) {setVideoSize(a->data().toDouble());});
    connect(win[u"minimize"_q], &QAction::triggered, p, &MainWindow::showMinimized);
    connect(win[u"maximize"_q], &QAction::triggered, p, &MainWindow::showMaximized);
    connect(win[u"close"_q], &QAction::triggered, p, [=] () { menu.hide(); p->close(); });

    Menu &help = menu(u"help"_q);
    connect(help[u"about"_q], &QAction::triggered,
            p, [=] () { AboutDialog dlg(p); dlg.exec(); });
    connect(menu[u"exit"_q], &QAction::triggered, p, &MainWindow::exit);

    plugStreamActions(&menu(u"play"_q)(u"title"_q), &PlayEngine::currentEdition);
    plugStreamActions(&menu(u"play"_q)(u"chapter"_q), &PlayEngine::currentChapter);
}
