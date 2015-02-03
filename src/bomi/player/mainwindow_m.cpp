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

template<class F>
auto MainWindow::Data::plugStreamActions(Menu *menu, F func,
                                         const QString &group) -> void
{
    auto checkCurrentStreamAction = [=] () {
        const auto current = (engine.*func)();
        for (auto action : menu->g(group)->actions()) {
            if (action->data().toInt() == current) {
                action->setChecked(true);
                break;
            }
        }
    };
    connect(menu, &QMenu::aboutToShow, p, checkCurrentStreamAction);
}

template<class T, class Func>
auto MainWindow::Data::push(const T &to, const T &from,
                            const Func &func) -> QUndoCommand*
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

template<class T, class F, class Handler, class State>
auto MainWindow::Data::plugEnumActions(Menu &menu, State &state,
                                       Handler handler, const char *asprop,
                                       Signal<State> sig, F f) -> void
{
    Q_ASSERT(state.property(asprop).isValid());
    const int size = EnumInfo<T>::size();
    auto cycle = _C(menu)[size > 2 ? u"next"_q : u"toggle"_q];
    auto group = menu.g(typeKey<T>());
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
    if (std::is_same<State, MrlState>::value)
        enumGroups.append(EnumGroup{asprop, group});
}

template<class T, class F, class S>
auto MainWindow::Data::plugEnumActions(Menu &menu, S &s, const char *asprop,
                                       Signal<S> sig, F f) -> void
{
    auto group = menu.g(typeKey<T>());
    auto ps = &s; auto pm = &menu;
    auto push = [=] (T t) {
        if (auto action = group->find(t)) {
            action->setChecked(true);
            showMessage(pm->title(), action->text());
            ps->setProperty(asprop, action->data());
        }
    };
    auto handler = [=] (QAction *a) {
        auto action = static_cast<EnumAction<T>*>(a);
        auto t = action->enum_();
        auto current = ps->property(asprop).template value<T>();
        if (current != t)
            this->push<T>(t, current, push);
    };
    plugEnumActions<T, F>(menu, s, handler, asprop, sig, f);
}

template<class T, class F, class GetNew, class S>
auto MainWindow::Data::plugEnumDataActions(Menu &menu, S &s, const char *prop,
                                           GetNew getNew, Signal<S> sig, F f) -> void
{
    using EData = typename EnumInfo<T>::Data;
    plugEnumActions<T, F>(menu, s, [=, &s, &menu] (QAction *a) {
        auto action = static_cast<EnumAction<T>*>(a);
        auto old = s.property(prop).template value<EData>();
        auto new_ = getNew(action->data());
        auto step = dynamic_cast<StepAction*>(action);
        if (step)
            new_ = step->clamp(new_);
        if (old != new_)
            this->push<EData>(new_, old, [=, &s, &menu] (const EData &data) {
                s.setProperty(prop, QVariant::fromValue<EData>(data));
                if (step)
                    showMessage(menu.title(), step->format(data));
            });
    }, prop, sig, f);
}

template<class F>
auto MainWindow::Data::plugStepActions(Menu &menu, const char *asprop,
                                       MSig sig, F f) -> void
{
    plugEnumDataActions<ChangeValue, F>(menu, asprop, [=] (int diff) {
        return diff ? (as.state.property(asprop).toInt() + diff) : 0;
    }, sig, f);
}

template<class T, class F, class GetNew>
auto MainWindow::Data::plugPropertyDiff(ActionGroup *g, const char *asprop,
                                        GetNew getNew, MSig sig, F f) -> void
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
auto MainWindow::Data::plugPropertyDiff(ActionGroup *g, const char *asprop,
                                        T min, T max, MSig sig, F f) -> void
{
    plugPropertyDiff<T, F>(g, asprop, [=] (QAction *a, const T &old) {
        return qBound<T>(min, a->data().value<T>() + old, max);
    }, sig, f);
}

template<class F>
auto MainWindow::Data::plugPropertyCheckable(QAction *a, const char *asprop,
                                             MSig sig, F f) -> void
{
    Q_ASSERT(as.state.property(asprop).isValid()
             && as.state.property(asprop).type() == QVariant::Bool);
    Q_ASSERT(a->isCheckable());
    connect(a, &QAction::triggered, p, [=] (bool new_) {
        const bool old = as.state.property(asprop).toBool();
        if (new_ != old) {
            push(new_, old, [=](bool checked) {
                as.state.setProperty(asprop, checked);
                a->setChecked(checked);
                showMessage(a->text(), checked);
            });
        }
    });
    connect(&as.state, sig, f);
    emit (as.state.*sig)();
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
            p, [this] () {engine.stop();});
    plugStepActions(play(u"speed"_q), "play_speed",
                          &MrlState::playSpeedChanged, [this]() {
        engine.setSpeed(1e-2*as.state.play_speed);
    });
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
            if (engine.isStopped())
                break;
            const auto t = engine.time();
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
            const auto time = engine.time();
            ab.setA(subtitle.start(time));
            ab.setB(subtitle.finish(time));
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
        engine.seek(engine.begin());
    });
    connect(seek.g(u"relative"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        const int diff = static_cast<StepAction*>(a)->data();
        if (diff && !engine.isStopped() && engine.isSeekable()) {
            engine.relativeSeek(diff);
            showMessage(tr("Seeking"), diff/1000, tr("sec"), true);
            showTimeLine();
        }
    });
    connect(seek.g(u"frame"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        engine.stepFrame(a->data().toInt());
    });
    connect(seek[u"black-frame"_q], &QAction::triggered, p, [=] () {
        engine.seekToNextBlackFrame();
        showMessage(tr("Seek to Next Black Frame"));
    });
    connect(play[u"disc-menu"_q], &QAction::triggered, p, [this] () {
        engine.setCurrentEdition(PlayEngine::DVDMenu);
    });
    connect(seek.g(u"subtitle"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        const int key = a->data().toInt();
        const int time = key < 0 ? subtitle.previous()
                       : key > 0 ? subtitle.next()
                                 : subtitle.current();
        if (time >= 0)
            engine.seek(time-100);
    });
    connect(play(u"title"_q).g(), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        a->setChecked(true);
        engine.setCurrentEdition(a->data().toInt());
        showMessage(tr("Current Title"), a->text());
    });
    connect(play(u"chapter"_q).g(), &ActionGroup::triggered, p,
            [this] (QAction *a) {
        a->setChecked(true);
        engine.setCurrentChapter(a->data().toInt());
        showMessage(tr("Current Chapter"), a->text());
    });
    auto seekChapter = [this] (int offset) {
        if (!engine.chapters().isEmpty()) {
            auto target = engine.currentChapter() + offset;
            if (target > -2)
                engine.setCurrentChapter(target);
        }
    };
    connect(play(u"chapter"_q)[u"prev"_q], &QAction::triggered,
            p, [seekChapter] () { seekChapter(-1); });
    connect(play(u"chapter"_q)[u"next"_q], &QAction::triggered, p,
            [seekChapter] () { seekChapter(+1); });

    connect(play[u"state"_q], &QAction::triggered, p, [=] () {
        QString str;
        str.append(_MSecToString(engine.time()));
        str.append(u"/"_q);
        str.append(_MSecToString(engine.end()));
        str.append(u" ("_q);
        str.append(QString::number(engine.time() * 100L / engine.end()));
        str.append(u"%), "_q);
        str.append(QString::number(engine.speed()));
        str.append(u"x"_q);
        showMessage(str);
    });

    Menu &video = menu(u"video"_q);
    connect(video(u"track"_q).g(), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        a->setChecked(true);
        engine.setCurrentVideoStream(a->data().toInt());
        showMessage(tr("Current Video Track"), a->text());
    });
    plugEnumActions<VideoRatio>(video(u"aspect"_q), "video_aspect_ratio",
                                &MrlState::videoAspectRatioChanged, [this] ()
        { vr.setAspectRatio(_EnumData(as.state.video_aspect_ratio)); });
    plugEnumActions<VideoRatio>(video(u"crop"_q), "video_crop_ratio",
                                &MrlState::videoCropRatioChanged, [this] ()
        { vr.setCropRatio(_EnumData(as.state.video_crop_ratio)); });

    auto &snap = video(u"snapshot"_q);
    auto connectSnapshot = [&] (const QString &actionName, SnapshotMode mode) {
        connect(snap[actionName], &QAction::triggered, p, [this, mode] () {
            snapshotMode = mode;
            if (snapshotMode == NoSnapshot)
                return;
            PlayEngine::Snapshot snapshot = PlayEngine::VideoOnly;
            if (snapshotMode == QuickSnapshot || snapshotMode == SnapshotTool)
                snapshot = PlayEngine::VideoAndOsd;
            engine.takeSnapshot(snapshot);
        });
    };
    connectSnapshot(u"quick"_q, QuickSnapshot);
    connectSnapshot(u"quick-nosub"_q, QuickSnapshotNoSub);
    connectSnapshot(u"tool"_q, SnapshotTool);
    connect(&engine, &PlayEngine::snapshotTaken, p, [this] () {
        auto video = engine.snapshot(false);
        auto osd = engine.snapshot(true);
        engine.clearSnapshots();
        if (video.isNull() && osd.isNull())
            return;
        if (snapshotMode == QuickSnapshot || snapshotMode == SnapshotTool) {
            QRectF subRect;
            auto sub = subtitle.draw(osd.rect(), &subRect);
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
                    if (vr.hasFrame()) {
                        snapshotMode = SnapshotTool;
                        engine.takeSnapshot(PlayEngine::VideoAndOsd);
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
                if (engine.mrl().isLocalFile()) {
                    file = _ToAbsPath(engine.mrl().toLocalFile())
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

    auto setVideoAlignment = [this] () {
        const auto v = _EnumData(as.state.video_vertical_alignment);
        const auto h = _EnumData(as.state.video_horizontal_alignment);
        vr.setAlignment(v | h);
    };
    plugEnumActions<VerticalAlignment>
            (video(u"align"_q), "video_vertical_alignment",
             &MrlState::videoVerticalAlignmentChanged, setVideoAlignment);
    plugEnumActions<HorizontalAlignment>
            (video(u"align"_q), "video_horizontal_alignment",
             &MrlState::videoHorizontalAlignmentChanged, setVideoAlignment);
    plugEnumDataActions<MoveToward>
            (video(u"move"_q), "video_offset",
             [this] (const QPoint &diff) {
        return diff.isNull() ? diff : as.state.video_offset + diff;
    }, &MrlState::videoOffsetChanged, [this] () {
        vr.setOffset(as.state.video_offset);
    });
    plugEnumMenu<DeintMode>
            (video, "video_deinterlacing",
             &MrlState::videoDeinterlacingChanged, [this] () {
        engine.setDeintMode(as.state.video_deinterlacing);
    });
    plugEnumActions<Interpolator>
            (video(u"interpolator"_q), "video_interpolator",
             &MrlState::videoInterpolatorChanged, [this] () {
        engine.setInterpolator(as.state.video_interpolator);
    });
    plugEnumActions<Interpolator>
            (video(u"chroma-upscaler"_q), "video_chroma_upscaler",
             &MrlState::videoChromaUpscalerChanged, [this] () {
        engine.setChromaUpscaler(as.state.video_chroma_upscaler);
    });
    auto updateHqScale = [this] () {
        engine.setHighQualityScaling(as.state.video_hq_upscaling,
                                     as.state.video_hq_downscaling);
    };
    plugPropertyCheckable(video(u"hq-scaling"_q)[u"up"_q], "video_hq_upscaling",
            &MrlState::videoHqUpscalingChanged, updateHqScale);
    plugPropertyCheckable(video(u"hq-scaling"_q)[u"down"_q], "video_hq_downscaling",
            &MrlState::videoHqDownscalingChanged, updateHqScale);

    plugEnumMenu<Dithering>
            (video, "video_dithering",
             &MrlState::videoDitheringChanged, [this] () {
        engine.setDithering(as.state.video_dithering);
    });
    plugEnumMenu<ColorSpace>(video, "video_space",
                                   &MrlState::videoSpaceChanged, [this] () {
        engine.setColorSpace(as.state.video_space);
    });
    plugEnumMenu<ColorRange>(video, "video_range",
                                   &MrlState::videoRangeChanged, [this] () {
        engine.setColorRange(as.state.video_range);
    });

    connect(&video(u"filter"_q), &Menu::triggered, p, [this] () {
        VideoEffects effects = 0;
        for (auto act : menu(u"video"_q)(u"filter"_q).actions()) {
            if (act->isChecked())
                effects |= act->data().value<VideoEffect>();
        }
        if (engine.videoEffects() != effects)
            push(effects, engine.videoEffects(),
                    [this] (VideoEffects effects) {
                engine.setVideoEffects(effects);
                as.state.video_effects = effects;
                for (auto a : menu(u"video"_q)(u"filter"_q).actions())
                    a->setChecked(a->data().value<VideoEffect>() & effects);
            });
    });
    VideoColor::for_type([=, &video] (VideoColor::Type type) {
        const auto name = VideoColor::name(type);
        connect(video(u"color"_q).g(name), &ActionGroup::triggered, p,
                [=] (QAction *a) {
            const int diff = static_cast<StepAction*>(a)->data();
            auto color = as.state.video_color;
            color.add(type, diff);
            const auto text = color.formatText(type).arg(color.get(type));
            showMessage(tr("Adjust Video Color"), text);
            as.state.setProperty("video_color", QVariant::fromValue(color));
        });
    });
    connect(video(u"color"_q)[u"reset"_q], &QAction::triggered, p, [this] () {
        showMessage(tr("Reset Video Color"));
        const auto var = QVariant::fromValue(VideoColor());
        as.state.setProperty("video_color", var);
    });
    connect(&as.state, &MrlState::videoColorChanged,
            &engine, &PlayEngine::setVideoEqualizer);

    Menu &audio = menu(u"audio"_q);
    connect(audio(u"track"_q).g(), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        a->setChecked(true);
        engine.setCurrentAudioStream(a->data().toInt());
        showMessage(tr("Current Audio Track"), a->text());
    });
    plugStepActions(audio(u"volume"_q), "audio_volume",
                          &MrlState::audioVolumeChanged, [this] () {
        engine.setVolume(as.state.audio_volume);
    });
    plugPropertyCheckable(audio(u"volume"_q)[u"mute"_q], "audio_muted",
                                &MrlState::audioMutedChanged, [this] () {
        engine.setMuted(as.state.audio_muted);
    });
    connect(&as.state, &MrlState::audioEqualizerChanged, &engine, &PlayEngine::setAudioEqualizer);
    connect(audio[u"equalizer"_q], &QAction::triggered, p, [=] () {
        if (!eq) {
            eq = new AudioEqualizerDialog(p);
            connect(eq, &AudioEqualizerDialog::equalizerChanged, p, [=] (const AudioEqualizer &eq)
                    { as.state.setProperty("audio_equalizer", QVariant::fromValue(eq)); });
        }
        eq->setEqualizer(as.state.audio_equalizer);
        eq->show();
    });
    plugStepActions(audio(u"sync"_q), "audio_sync",
                          &MrlState::audioSyncChanged, [this] () {
        engine.setAudioSync(as.state.audio_sync);
    });
    plugStepActions(audio(u"amp"_q), "audio_amplifier",
                          &MrlState::audioAmpChanged, [this] () {
        engine.setAmp(as.state.audio_amplifier*1e-2);
    });
    plugEnumMenu<ChannelLayout>
            (audio, "audio_channel_layout",
             &MrlState::audioChannelLayoutChanged, [this] () {
        engine.setChannelLayout(as.state.audio_channel_layout);
    });
    plugPropertyCheckable
            (audio[u"normalizer"_q], "audio_volume_normalizer",
             &MrlState::audioVolumeNormalizerChanged, [this] () {
        const auto activate = as.state.audio_volume_normalizer;
        engine.setVolumeNormalizerActivated(activate);
    });
    plugPropertyCheckable
            (audio[u"tempo-scaler"_q], "audio_tempo_scaler",
             &MrlState::audioTempoScalerChanged, [this] () {
        engine.setTempoScalerActivated(as.state.audio_tempo_scaler);
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
    connect(audio(u"track"_q)[u"next"_q], &QAction::triggered, p, [=] ()
        { selectNext(menu(u"audio"_q)(u"track"_q).g()->actions()); });

    Menu &sub = menu(u"subtitle"_q);
    auto &strack = sub(u"track"_q);
    subtrackSep = strack.addSeparator();
    connect(strack[u"next"_q], &QAction::triggered, p, [this, &strack] () {
        int checked = -1;
        auto list = strack.g(u"external"_q)->actions();
        list += strack.g(u"internal"_q)->actions();
        if (list.isEmpty() || (list.size() == 1 && list.first()->isChecked()))
            return;
        changingSub = true;
        for (int i=0; i<list.size(); ++i) {
            if (list[i]->isChecked()) {
                checked = i;
                list[i]->setChecked(false);
            }
        }
        changingSub = false;
        subtitle.deselect(-1);
        if (++checked >= list.size())
            checked = 0;
        list[checked]->trigger();
        if (!strack.g(u"internal"_q)->checkedAction())
            engine.setCurrentSubtitleStream(-2);
    });
    connect(strack[u"all"_q], &QAction::triggered, p, [=, &strack] () {
        subtitle.select(-1);
        const auto acts = strack.g(u"external"_q)->actions();
        for (auto action : acts)
            action->setChecked(true);
        const int count = subtitle.componentsCount();
        const auto text = tr("%1 Subtitle(s)").arg(count);
        showMessage(tr("Select All Subtitles"), text);
        setSubtitleTracksToEngine();
    });
    connect(strack[u"hide"_q], &QAction::triggered,
            p, [this, &strack] (bool hide) {
        if (hide != subtitle.isHidden()) {
            push(hide, subtitle.isHidden(), [this, &strack] (bool hide) {
                subtitle.setHidden(hide);
                engine.setSubtitleStreamsVisible(!hide);
                if (hide)
                    showMessage(tr("Hide Subtitles"));
                else
                    showMessage(tr("Show Subtitles"));
                strack[u"hide"_q]->setChecked(hide);
            });
        }
    });
    connect(strack[u"open"_q], &QAction::triggered, p, [this] () {
        QString dir;
        if (engine.mrl().isLocalFile())
            dir = _ToAbsPath(engine.mrl().toLocalFile());
        QString enc = pref().sub_enc;
        const auto files = EncodingFileDialog::getOpenFileNames
                                    (p, tr("Open Subtitle"), dir,
                                     _ToFilter(SubtitleExt), &enc);
        if (!files.isEmpty())
            appendSubFiles(files, true, enc);
    });
    connect(strack[u"auto-load"_q], &QAction::triggered, p, [this] () {
        clearSubtitleFiles();
        updateSubtitleState();
    });
    connect(strack[u"reload"_q], &QAction::triggered, p, [this] () {
        auto state = subtitleState();
        clearSubtitleFiles();
        setSubtitleState(state);
    });
    connect(strack[u"clear"_q], &QAction::triggered,
            p, [=] () { clearSubtitleFiles(); });
    connect(strack.g(u"external"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        if (!changingSub) {
            if (a->isChecked())
                subtitle.select(a->data().toInt());
            else
                subtitle.deselect(a->data().toInt());
        }
        showMessage(tr("Selected Subtitle"), a->text());
        setSubtitleTracksToEngine();
    });
    connect(strack.g(u"internal"_q), &ActionGroup::triggered,
            p, [this, &strack] (QAction *a) {
        const bool checked = a->isChecked();
        auto actions = strack.g(u"internal"_q)->actions();
        for (auto action : actions)
            action->setChecked(false);
        a->setChecked(checked);
        if (checked) {
            engine.setCurrentSubtitleStream(a->data().toInt());
            showMessage(tr("Selected Subtitle"), a->text());
        } else
            engine.setCurrentSubtitleStream(-1);
        setSubtitleTracksToEngine();
    });
//    connect(&strack, &Menu::actionsSynchronized, p, [this] () {
//        setSubtitleTracksToEngine();
//    });
    plugEnumMenu<SubtitleDisplay>
            (sub, "sub_display", &MrlState::subDisplayChanged, [this] () {
        const auto on = as.state.sub_display == SubtitleDisplay::OnLetterbox;
        vr.setOverlayOnLetterbox(on);
    });
    plugEnumActions<VerticalAlignment>
            (sub(u"align"_q), "sub_alignment",
             &MrlState::subAlignmentChanged, [this] () {
        const auto top = as.state.sub_alignment == VerticalAlignment::Top;
        subtitle.setTopAligned(top);
    });
    plugStepActions(sub(u"position"_q), "sub_position",
                          &MrlState::subPositionChanged, [this] () {
        subtitle.setPos(as.state.sub_position*1e-2);
    });
    plugStepActions(sub(u"sync"_q), "sub_sync",
                          &MrlState::subSyncChanged, [this] () {
        subtitle.setDelay(as.state.sub_sync);
        engine.setSubtitleDelay(as.state.sub_sync);
    });

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
            prefDlg->setAudioDeviceList(engine.audioDeviceList());
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
                appendSubFiles(QStringList(fileName), true, pref().sub_enc);
                showMessage(tr("Downloaded"), QFileInfo(fileName).fileName());
            });
        }
        subFindDlg->find(engine.mrl());
        subFindDlg->show();
    });
    connect(tool[u"reload-skin"_q], &QAction::triggered,
            p, [=] () { reloadSkin(); });
    connect(tool[u"auto-exit"_q], &QAction::triggered, p, [this] (bool on) {
        if (on != as.auto_exit)
            push(on, as.auto_exit, [this] (bool on) {
                as.auto_exit = on;
                showMessage(on ? tr("Exit bomi when "
                                    "the playlist has finished.")
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
    plugEnumMenu<StaysOnTop>
            (win, as, "win_stays_on_top", &AppState::winStaysOnTopChanged,
             [this] () { updateStaysOnTop(); });
    connect(win.g(u"size"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) {setVideoSize(a->data().toDouble());});
    connect(win[u"minimize"_q], &QAction::triggered, p, &MainWindow::showMinimized);
    connect(win[u"maximize"_q], &QAction::triggered, p, &MainWindow::showMaximized);
    connect(win[u"close"_q], &QAction::triggered, p,
            [=] () { menu.hide(); p->close(); });

    Menu &help = menu(u"help"_q);
    connect(help[u"about"_q], &QAction::triggered,
            p, [=] () { AboutDialog dlg(p); dlg.exec(); });
    connect(menu[u"exit"_q], &QAction::triggered, p, &MainWindow::exit);

    plugStreamActions(&menu(u"play"_q)(u"title"_q), &PlayEngine::currentEdition);
    plugStreamActions(&menu(u"play"_q)(u"chapter"_q), &PlayEngine::currentChapter);
}
