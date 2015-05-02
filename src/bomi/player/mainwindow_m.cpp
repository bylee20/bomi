#include "mainwindow_p.hpp"
#include "app.hpp"
#include "enum/movetoward.hpp"
#include "player/videosettings.hpp"
#include "misc/actiongroup.hpp"
#include "subtitle/subtitleviewer.hpp"
#include "subtitle/subtitlemodel.hpp"
#include "dialog/fileassocdialog.hpp"
#include "dialog/mbox.hpp"
#include "dialog/audioequalizerdialog.hpp"
#include "dialog/videocolordialog.hpp"
#include "dialog/urldialog.hpp"
#include "dialog/openmediafolderdialog.hpp"
#include "pref/prefdialog.hpp"
#include "dialog/aboutdialog.hpp"
#include "dialog/opendiscdialog.hpp"
#include "dialog/snapshotdialog.hpp"
#include "dialog/subtitlefinddialog.hpp"
#include "dialog/encodingfiledialog.hpp"
#include "dialog/encoderdialog.hpp"
#include "video/interpolatorparams.hpp"
#include "audio/visualizer.hpp"
#include "misc/filenamegenerator.hpp"
#include <QThreadPool>
#include <QClipboard>

template<class T, class Func>
auto MainWindow::Data::push(const T &to, const T &from, const Func &func) -> QUndoCommand*
{
    if (undo.isActive()) {
        auto cmd = new ValueCmd<Func, T>(to, from, func);
        undo.push(cmd);
        return cmd;
    } else {
        func(to);
        return nullptr;
    }
}

template<class T, class S>
auto MainWindow::Data::push(const T &to, const T &old, void(PlayEngine::*set)(S)) -> QUndoCommand*
{
    if (to == old)
        return nullptr;
    return push(to, old, [=] (const T &s) { (e.*set)(s); });
}

template<class T, class S>
auto MainWindow::Data::push(const T &to, T(MrlState::*get)() const, void(PlayEngine::*set)(S)) -> QUndoCommand*
{
    return push(to, (e.params()->*get)(), set);
}

auto MainWindow::Data::plugFlag(QAction *action, bool(MrlState::*get)() const,
                                void(MrlState::*sig)(bool),
                                QString(MrlState::*desc)() const,
                                void(PlayEngine::*set)(bool)) -> void
{
    connect(e.params(), sig, action, &QAction::setChecked);
    connect(action, &QAction::triggered, &e, [=] (bool on) { push(on, get, set); });
    propertyMessage(desc, sig);
}

template<class T>
auto MainWindow::Data::plugStep(ActionGroup *g, PLUG_MEMFUNC(T)) -> void
{
    auto &step = static_cast<StepAction*>(g->find(ChangeValue::Increase))->value();
    connect(g, &ActionGroup::triggered, p, [=, &step] (QAction *a) {
        auto action = qobject_cast<StepAction*>(a);
        Q_ASSERT(action);
        T value = T();
        if (action->isReset())
            value = (e.default_()->*get)();
        else
            value = step.changed((e.params()->*get)(), action->enum_());
        push(value, get, set);
    });
    propertyMessage(desc, sig, [=, &step] (T val) { return step.text(val); });
}

template<class T>
auto MainWindow::Data::plugAppEnumChild(Menu &parent, const char *prop,
                                        void(AppState::*sig)(T)) -> void
{
    auto m = &parent(_L(EnumInfo<T>::typeKey()));
    auto g = m->g(_L(EnumInfo<T>::typeKey()));
    connect(&as, sig, g, &ActionGroup::setChecked<T>);
    connect(g, &ActionGroup::triggered, p, [=] (QAction *a) {
        const auto old = as.property(prop).value<T>();
        const auto to = a->data().template value<T>();
        showMessage(m->title(), EnumInfo<T>::description(to));
        if (to != old)
            push(to, old, [=] (T t) {
                as.setProperty(prop, QVariant::fromValue<T>(t));
                showMessage(m->title(), EnumInfo<T>::description(t));
            });
    });
    if (auto cycle = m->action(u"cycle"_q))
        plugCycle(cycle, g);
}

template<class T>
auto MainWindow::Data::plugEnum(ActionGroup *g, PLUG_MEMFUNC(T), QAction *cycle) -> void
{
    connect(e.params(), sig, g, &ActionGroup::setChecked<T>);
    connect(g, &ActionGroup::triggered, p,
            [=] (QAction *a) { push(a->data().template value<T>(), get, set); });
    if (cycle)
        connect(cycle, &QAction::triggered, p,
                [=] () { triggerNextAction(g->actions()); });
    propertyMessage(desc, sig, [=] (T val) { return EnumInfo<T>::description(val); });
}

auto getNextAction(const QList<QAction*> &actions) -> QAction*
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
        return actions[i];
    return nullptr;
}


auto MainWindow::Data::triggerNextAction(const QList<QAction*> &actions) -> void
{
    if (auto action = getNextAction(actions))
        action->trigger();
}

auto MainWindow::Data::plugTrack(Menu &parent, StreamType type,
                                 const QString &gkey, QAction *sep) -> void
{
    auto *m = &parent(u"track"_q);
    const auto g = m->g(gkey);
    connect(e.params(), e.params()->signal(type), p, [=] (StreamList list) {
        g->clear(); if (list.isEmpty()) return;
        for (auto it = list.begin(); it != list.end(); ++it) {
            auto act = new QAction(it->name(), m);
            m->insertAction(sep, act);
            g->addAction(act);
            act->setCheckable(true);
            act->setData(it->id());
            act->setChecked(it->isSelected());
        }
    });
    connect(g, &ActionGroup::triggered, p, [=] (QAction *a)
        { e.setTrackSelected(type, a->data().toInt(), a->isChecked()); });
}

auto MainWindow::Data::plugMenu() -> void
{
    ActionGroup *g = nullptr;

    Menu &open = menu(u"open"_q);
    connect(open[u"file"_q], &QAction::triggered, p, [this] () {
        const auto file = _GetOpenFile(nullptr, tr("Open File"), MediaExt | PlaylistExt);
        if (!file.isEmpty()) {
            const Mrl mrl(file);
            if (_IsSuffixOf(PlaylistExt, mrl.suffix())) {
                Playlist list;
                if (list.load(file))
                    playlist.setList(list);
            } else
                openMrl(mrl);
        }
    });
    connect(open[u"folder"_q], &QAction::triggered, p, [this] () { openDir(); });
    connect(open[u"url"_q], &QAction::triggered, p, [this] () {
        auto dlg = dialog<UrlDialog>();
        if (dlg->exec()) {
            downloader.cancel();
            if (dlg->isPlaylist())
                playlist.open(dlg->url(), dlg->encoding());
            else
                openMrl(dlg->url().toString());
        }
    });
    auto openDisc = [this] (const QString &title, QString &device, bool dvd) {
        auto dlg = dialog<OpenDiscDialog>();
        dlg->setIsoEnabled(dvd);
        _SetWindowTitle(dlg.data(), title);
        dlg->setDeviceList(OS::opticalDrives());
        if (!device.isEmpty())
            dlg->setDevice(device);
        if (dlg->exec())
            device = dlg->device();
        return dlg->result() && !device.isEmpty();
    };

    connect(open[u"dvd"_q], &QAction::triggered, p, [openDisc, this] () {
        if (openDisc(tr("Select DVD device"), as.dvd_device, true))
            openMrl(Mrl::fromDisc(u"dvdnav"_q, as.dvd_device, -1, true));
    });
    connect(open[u"bluray"_q], &QAction::triggered, p, [openDisc, this] () {
        if (openDisc(tr("Select Blu-ray device"), as.bluray_device, false))
            openMrl(Mrl::fromDisc(u"bdnav"_q, as.bluray_device, -1, true));
    });
    connect(open[u"clipboard"_q], &QAction::triggered,
            p, [=] () { openMimeData(qApp->clipboard()->mimeData()); });
    connect(open(u"recent"_q).g(), &ActionGroup::triggered,
            p, [this] (QAction *a) {openMrl(Mrl(a->data().toString()));});
    connect(open(u"recent"_q)[u"clear"_q], &QAction::triggered,
            &recent, &RecentInfo::clear);

    Menu &play = menu(u"play"_q);
    connect(play[u"stop"_q], &QAction::triggered, &e, &PlayEngine::stop);
    PLUG_STEP(play(u"speed"_q).g(), play_speed, setSpeed);

    connect(play[u"play-pause"_q], &QAction::triggered, p, &MainWindow::togglePlayPause);
    connect(play[u"play-stop"_q], &QAction::triggered, p, &MainWindow::togglePlayStop);
    connect(play[u"play"_q], &QAction::triggered, p, &MainWindow::play);
    connect(play[u"pause"_q], &QAction::triggered, p, &MainWindow::pause);
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
    connect(play[u"prev"_q], &QAction::triggered, &playlist, &PlaylistModel::playPrevious);
    connect(play[u"next"_q], &QAction::triggered, &playlist, &PlaylistModel::playNext);

    auto &seek = play(u"seek"_q);
    connect(seek[u"begin"_q], &QAction::triggered, p, [=] () { e.seek(e.begin()); });
    connect(seek[u"record"_q], &QAction::triggered, p, [=] ()
        { const int pos = e.params()->resume_position(); if (pos > 0) e.seek(pos); });
    connect(seek.g(u"relative"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) {
        const auto action = static_cast<StepAction*>(a);
        const int diff = action->value().changed(0, action->enum_());
        if (diff && !e.isStopped() && e.isSeekable()) {
            e.relativeSeek(diff);
            showMessage(tr("Seeking"), a->text());
            showTimeLine();
        }
    });
    connect(seek.g(u"frame"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) { e.stepFrame(a->data().toInt()); });
    connect(seek[u"black-frame"_q], &QAction::triggered, p, [=] () {
        e.seekToNextBlackFrame();
        showMessage(tr("Seek to Next Black Frame"));
    });
    connect(play[u"disc-menu"_q], &QAction::triggered,
            p, [=] () { e.seekEdition(PlayEngine::DVDMenu); });
    connect(seek.g(u"subtitle"_q), &ActionGroup::triggered,
            p, [this] (QAction *a) { e.seek(e.captionBeginTime(a->data().toInt())); });

    auto seekChapter = [this] (int offset) {
        if (!e.chapters().isEmpty()) {
            auto target = e.chapter()->number() + offset;
            if (target > -2)
                e.seekChapter(target);
        }
    };
    connect(play(u"chapter"_q)[u"prev"_q], &QAction::triggered,
            p, [seekChapter] () { seekChapter(-1); });
    connect(play(u"chapter"_q)[u"next"_q], &QAction::triggered, p,
            [seekChapter] () { seekChapter(+1); });

    connect(play[u"state"_q], &QAction::triggered, p, [=] () {
        const auto fmt = u"%1/%2 (%3%), %4x"_q;
        showMessage(fmt.arg(_MSecToString(e.time()), _MSecToString(e.end()),
                            _N(e.rate() * 100, 1), _N(e.speed(), 2)));
    });
    g = play(u"streaming"_q).g();
    connect(g, &ActionGroup::triggered, p,
            [=] (QAction *a) { e.setStreamingFormat(a->data().toString()); });
    connect(&e, &PlayEngine::streamingFormatsChanged, p, [=, &play] () {
        g->clear();
        auto &m = play(u"streaming"_q);
        const auto current = e.streamingFormat()->id();
        const auto list = e.streamingFormats();
        for (auto fmt : list) {
            auto action = m.addActionNoKey(fmt->name(), true);
            action->setData(fmt->id());
            action->setChecked(fmt->id() == current);
        }
        m.setEnabled(!list.isEmpty());
    });

    Menu &video = menu(u"video"_q);

    auto ratio = &video(u"aspect"_q);
#define PLUG_RATIO(prop, s) g = ratio->g(u"preset"_q); {\
    PROP_NOTIFY(prop, [=] (double v) { \
        if (auto checked = g->setChecked(v)) return checked->text(); \
        const auto size = e.videoSizeHint(); \
        return u"%3:1 (%1x%2)"_q.arg(size.width()).arg(size.height()).arg(v, 0, 'g', 6); }); \
    connect(g, &ActionGroup::triggered, p, [=] (QAction *a) \
        {  push(a->data().toDouble(), e.params()->prop(), [=] (double v) { e.s(v); }); }); \
    plugCycle(*ratio, g); }

    PLUG_RATIO(video_aspect_ratio, setVideoAspectRatio);
    g = ratio->g(u"adjust"_q);
    connect(g, &ActionGroup::triggered, p, [=] (QAction *a)
    {
        const auto r = e.videoOutputAspectRatio();
        auto action = static_cast<StepAction*>(a);
        push(action->value().changed(r, action->enum_()),
             e.params()->video_aspect_ratio(),
             [=] (double v) { e.setVideoAspectRatio(v); });
    });

    ratio = &video(u"crop"_q);
    PLUG_RATIO(video_crop_ratio, setVideoCropRatio);
    PLUG_ENUM_CHILD(video, video_rotation, setVideoRotation);

    auto &snap = video(u"snapshot"_q);
    auto connectSnapshot = [&] (const QString &actionName, SnapshotMode mode) {
        connect(snap[actionName], &QAction::triggered, p, [this, mode] ()
            { if ((snapshotMode = mode) != NoSnapshot) e.takeSnapshot(); });
    };
    connectSnapshot(u"quick"_q, QuickSnapshot);
    connectSnapshot(u"quick-nosub"_q, QuickSnapshotNoSub);
    connectSnapshot(u"tool"_q, SnapshotTool);
    connect(&e, &PlayEngine::snapshotTaken, p, [this] () {
        QImage frameOnly, withOsd;
        ph.position = _MSecToTime(e.snapshot(&frameOnly, &withOsd));
        e.clearSnapshots();
        if (frameOnly.isNull())
            return;
        if (snapshotMode == QuickSnapshot || snapshotMode == SnapshotTool) {
            QRectF subRect;
            QImage osd; osd.swap(withOsd);
            withOsd = frameOnly;
            QPainter painter(&withOsd);
            painter.drawImage(osd.rect(), osd);
            auto sub = e.subtitleImage(withOsd.rect(), &subRect);
            if (!sub.isNull())
                painter.drawImage(subRect, sub);
        }
        switch (snapshotMode) {
        case SnapshotTool: {
            if (!snapshot) {
                snapshot = dialog<SnapshotDialog>();
                snapshot->setTakeFunc([=] () {
                    if (e.hasVideoFrame()) {
                        snapshotMode = SnapshotTool;
                        e.takeSnapshot();
                    } else
                        snapshot->clear();
                });
            }
            if (!frameOnly.isNull())
                snapshot->setImage(frameOnly, withOsd);
            else
                snapshot->clear();
            break;
        } case QuickSnapshot: case QuickSnapshotNoSub: {
            auto image = frameOnly;
            if (snapshotMode == QuickSnapshot)
                image = withOsd;

            QString folder; bool ask = false;
            switch (pref.quick_snapshot_save()) {
            case QuickSnapshotSave::Current:
                if (e.mrl().isLocalFile()) {
                    folder = _ToAbsPath(e.mrl().toLocalFile());
                    break;
                }
            case QuickSnapshotSave::Ask:
                folder = _LastOpenPath();
                ask = true;
                break;
            case QuickSnapshotSave::Fixed:
                folder = pref.quick_snapshot_folder();
                break;
            default:
                return;
            }
            if (folder.isEmpty())
                return;
            const auto g = fileNameGenerator();
            auto file = g.get(folder, pref.quick_snapshot_template(), pref.quick_snapshot_format());
            if (ask)
                file = _GetSaveFile(nullptr, tr("Save Snapshot"), file, WritableImageExt);
            if (file.isEmpty())
                return;
            const int quality = pref.quick_snapshot_quality();
            const auto saver = new SnapshotSaver(image, file, quality);
            if (saver->isWritable()) {
                QThreadPool::globalInstance()->start(saver);
                showMessage(u"Save Snapshot"_q, file);
            } else {
                delete saver;
                MBox::error(nullptr, tr("Error"), tr("Failed to create next file:\n%1").arg(file), {BBox::Ok});
            }
            break;
        } default:
            break;
        }
    }, Qt::QueuedConnection);
    connect(video(u"clip"_q).g(), &ActionGroup::triggered, p, [=] (QAction *act) {
        if (!encoder) {
            encoder = dialog<EncoderDialog>();
            connect(encoder.data(), &EncoderDialog::cropAreaChanged, p, [=] (const QRect &rect) {
                if (cropbox) {
                    cropbox->setProperty("cx", rect.x());
                    cropbox->setProperty("cy", rect.y());
                    cropbox->setProperty("cw", rect.width());
                    cropbox->setProperty("ch", rect.height());
                    QMetaObject::invokeMethod(cropbox, "updateArea");
                }
            });
            connect(encoder.data(), &EncoderDialog::visibleChanged, p, [=] (bool visible) {
               if (cropbox)
                   cropbox->setVisible(visible);
            });
        }
        auto setup = [&] (int a, int b) {
            encoder->setSource(e.currentVideoStreamName(),
                               e.frameSize(), fileNameGenerator());
            encoder->setAudio(e.currentAudioStreamTrack());
            encoder->setSubtitle(e.currentSubtitleStreamTrack(), pref.sub_style());
            if (a >= 0)
                encoder->setRange(a, b);
            encoder->show();
        };

        const auto t = e.time();
        switch (act->data().toInt()) {
        case 'r': {
            if (e.isStopped())
                break;
            if (encoder->isBusy()) {
                showMessage(tr("Video Clip"), tr("Already working"));
                encoder->show();
                break;
            }
            static int a = -1, b = -1;
            if (a < 0) {
                a = t;
                showMessage(tr("Video Clip"), tr("Start from %1").arg(_MSecToString(a, u"hh:mm:ss.zzz"_q)));
            } else if (b < 0) {
                b = t;
                if (t  - a < 100) {
                    showMessage(tr("Video Clip"), tr("Range is too short!"));
                } else {
                    showMessage(tr("Video Clip"), tr("End at %1").arg(_MSecToString(b, u"hh:mm:ss.zzz"_q)));
                    setup(a, b);
                }
                a = b = -1;
            }
            break;
        } case 'a': {
            setup(-1, -1);
            break;
        } default:
            return;
        }
    });

    PLUG_ENUM(video(u"align"_q), video_vertical_alignment, setVideoVerticalAlignment);
    PLUG_ENUM(video(u"align"_q), video_horizontal_alignment, setVideoHorizontalAlignment);

    auto &move = video(u"move"_q);
    connect(move[u"reset"_q], &QAction::triggered, p, [=] ()
        { push(QPointF(), &MrlState::video_offset, &PlayEngine::setVideoOffset); });
#define PLUG_XY(grp, coord) connect(move.g(grp), &ActionGroup::triggered, p, [=] (QAction *a) { \
        const auto action = static_cast<StepAction*>(a); \
        QPointF offset = e.params()->video_offset(); \
        offset.r##coord() = action->value().changed(offset.coord(), action->enum_()); \
        push(offset, &MrlState::video_offset, &PlayEngine::setVideoOffset); })
    PLUG_XY(u"horizontal"_q, x); PLUG_XY(u"vertical"_q, y);
    PROP_NOTIFY(video_offset, [=] (const QPointF &offset) {
        const auto &step = pref.steps().video_offset_pct;
        return u"x: %1, y: %2"_q.arg(step.text(offset.x()), step.text(offset.y()));
    });

    PLUG_STEP(video(u"zoom"_q).g(), video_zoom, setVideoZoom);

    auto &vq = video(u"preset"_q);
    auto plugVideoPreset = [&] (const QString &key, VideoSettings::Preset preset)
    {
        auto action = vq[key];
        connect(action, &QAction::triggered, p, [=] () {
            auto from = e.videoSettings(); from.name = tr("Restored");
            auto to = VideoSettings::preset(preset); to.name = action->text();
            push(to, from, [=] (const VideoSettings &s) {
                showMessage(tr("Video Quality Settings"), s.name);
                auto prev = noMessage;
                noMessage = true;
                e.setVideoSettings(s);
                noMessage = prev;
            });
        });
    };
    plugVideoPreset(u"highest"_q, VideoSettings::Highest);
    plugVideoPreset(u"high"_q, VideoSettings::High);
    plugVideoPreset(u"normal"_q, VideoSettings::Normal);
    plugVideoPreset(u"basic"_q, VideoSettings::Basic);

    plugAppEnumChild(video, "fbo_format", &AppState::fboFormatChanged);
    connect(&as, &AppState::fboFormatChanged, &e, &PlayEngine::setFramebufferObjectFormat);
    connect(&e, &PlayEngine::framebufferObjectFormatChanged, p, [=] (auto f)
        { as.setProperty("fbo_format", QVariant::fromValue(f)); });

    PLUG_ENUM_CHILD(video, video_deinterlacing, setDeintMode);
    PLUG_ENUM(video(u"interpolator"_q), video_interpolator, setInterpolator);
    PLUG_ENUM(video(u"interpolator-down"_q), video_interpolator_down, setInterpolatorDown);
    PLUG_ENUM(video(u"chroma-upscaler"_q), video_chroma_upscaler, setChromaUpscaler);

#define PLUG_INTRPL(m, name, dlg, title, setParams, getParams, getMap) \
        connect(m[u"advanced"_q], &QAction::triggered, p, [=] () { \
            if (!dlg) { \
                dlg = dialog<IntrplDialog>(); \
                connect(dlg.data(), &IntrplDialog::paramsChanged, &e, \
                    static_cast<Signal<PlayEngine, const IntrplParamSet&>>(&PlayEngine::setParams)); \
                connect(e.params(), &MrlState::video_##name##_changed, dlg.data(), [=] () { \
                    if (dlg->isVisible()) \
                        dlg->set(e.getParams().type(), e.getMap()); \
                }); \
            } \
            if (!dlg->isVisible()) { \
                _SetWindowTitle(dlg.data(), title); \
                dlg->set(e.getParams().type(), e.getMap()); \
                dlg->show(); \
            } \
        });
    PLUG_INTRPL(video(u"interpolator-down"_q), interpolator_down, intrplDown,
                tr("Advanced Downscaling Interpolator Settings"),
                setInterpolatorDown, interpolatorDown, interpolatorDownMap);
    PLUG_INTRPL(video(u"interpolator"_q), interpolator, intrpl,
                tr("Advanced Interpolator Settings"),
                setInterpolator, interpolator, interpolatorMap);
    PLUG_INTRPL(video(u"chroma-upscaler"_q), chroma_upscaler, chroma,
                tr("Advanced Chroma Upscaler Settings"),
                setChromaUpscaler, chromaUpscaler, chromaUpscalerMap);

    const auto &dscale = video(u"interpolator-down"_q);
    const auto downSame = dscale[u"same"_q];
    connect(&as, &AppState::useInterpolatorDownChanged, p, [=, &dscale] (bool use) {
        e.setUseInterpolatorDown(use);
        downSame->setChecked(!use);
        dscale.g(_L(InterpolatorInfo::typeKey()))->setEnabled(use);
    });
    connect(downSame, &QAction::triggered, p, [=] (bool checked)
        { as.setProperty("use_interpolator_down", !checked); });

    PLUG_FLAG(video(u"hq-scaling"_q)[u"down"_q], video_hq_downscaling, setVideoHighQualityDownscaling);
    PLUG_FLAG(video(u"hq-scaling"_q)[u"up"_q], video_hq_upscaling, setVideoHighQualityUpscaling);
    PLUG_FLAG(video[u"motion"_q], video_motion_interpolation, setMotionInterpolation);

    PLUG_ENUM_CHILD(video, video_dithering, setVideoDithering);
    PLUG_ENUM_CHILD(video, video_space, setColorSpace);
    PLUG_ENUM_CHILD(video, video_range, setColorRange);

    connect(&video(u"filter"_q), &Menu::triggered, p, [this] () {
        VideoEffects effects = 0;
        for (auto act : menu(u"video"_q)(u"filter"_q).actions()) {
            if (act->isChecked())
                effects |= act->data().value<VideoEffect>();
        }
        push(effects, &MrlState::video_effects, &PlayEngine::setVideoEffects);
    });
    PROP_NOTIFY(video_effects, [=] (VideoEffects e) {
        for (auto a : menu(u"video"_q)(u"filter"_q).actions())
            a->setChecked(a->data().value<VideoEffect>() & e);
        return e.description();
    });

    auto &vcolor = video(u"color"_q);
    connect(vcolor[u"editor"_q], &QAction::triggered, p, [=] () {
        if (!color) {
            color = dialog<VideoColorDialog>();
            color->setUpdateFunc([=] (auto &eq) { e.setVideoEqualizer(eq); });
        }
        color->setColor(e.params()->video_color());
        color->show();
    });
    VideoColor::for_type([=, &vcolor] (VideoColor::Type type) {
        const auto name = VideoColor::name(type);
        connect(vcolor.g(name), &ActionGroup::triggered, p, [=] (QAction *a) {
            const int diff = static_cast<StepAction*>(a)->data();
            auto color = e.params()->video_color();
            color.add(type, diff);
            push(color, &MrlState::video_color, &PlayEngine::setVideoEqualizer);
        });
    });
    connect(vcolor[u"reset"_q], &QAction::triggered, p, [=] ()
        { push(VideoColor(), &MrlState::video_color, &PlayEngine::setVideoEqualizer); });
    PROP_NOTIFY(video_color, [=] (auto eq) { return eq.description(); });

    connect(e.params(), &MrlState::currentTrackChanged, p, [=] (StreamType type) {
        if (auto track = e.params()->tracks(type).selection())
            showMessage(e.params()->description(type), track->name());
        else
            showMessage(e.params()->description(type), false);
    });
    plugTrack(video, StreamVideo);

    Menu &audio = menu(u"audio"_q);

    PLUG_STEP(audio(u"volume"_q).g(), audio_volume, setAudioVolume);
    PLUG_FLAG(audio(u"volume"_q)[u"mute"_q], audio_muted, setAudioMuted);
    connect(audio[u"equalizer"_q], &QAction::triggered, p, [=] () {
        if (!eq) {
            eq = dialog<AudioEqualizerDialog>();
            eq->setUpdateFunc([=] (auto &eq) { e.setAudioEqualizer(eq); });
        }
        eq->setEqualizer(e.params()->audio_equalizer());
        eq->show();
    });
    PLUG_STEP(audio(u"sync"_q).g(), audio_sync, setAudioSync);
    PLUG_STEP(audio(u"amp"_q).g(), audio_amplifier, setAudioAmp);
    PLUG_ENUM_CHILD(audio, audio_channel_layout, setChannelLayout);
    plugAppEnumChild(audio, "visualization", &AppState::visualizationChanged);
    connect(&as, &AppState::visualizationChanged, e.visualizer(), &AudioVisualizer::setType);
    PLUG_FLAG(audio[u"normalizer"_q], audio_volume_normalizer, setAudioVolumeNormalizer);
    PLUG_FLAG(audio[u"tempo-scaler"_q], audio_tempo_scaler, setAudioTempoScaler);

    auto &atrack = audio(u"track"_q);
    plugTrack(audio, StreamAudio); plugCycle(atrack);
    connect(atrack[u"open"_q], &QAction::triggered, p, [this] () {
        const auto files = _GetOpenFiles(nullptr, tr("Open Audio File"), AudioExt);
        if (!files.isEmpty()) e.addAudioFiles(files);
    });
    connect(atrack[u"auto-load"_q], &QAction::triggered, &e, &PlayEngine::autoloadAudioFiles);
    connect(atrack[u"reload"_q], &QAction::triggered, &e, &PlayEngine::reloadAudioFiles);
    connect(atrack[u"clear"_q], &QAction::triggered, &e, &PlayEngine::clearAudioFiles);

    Menu &sub = menu(u"subtitle"_q);
    auto &strack = sub(u"track"_q);
    auto ssep = strack.addSeparator();
    plugTrack(sub, StreamSubtitle, "exclusive"_a);
    plugTrack(sub, StreamInclusiveSubtitle, "inclusive"_a, ssep);
    connect(sub(u"track"_q)[u"cycle"_q], &QAction::triggered, p, [=] () {
        auto &m = menu(u"subtitle"_q)(u"track"_q);
        auto actions = m.g(u"exclusive"_q)->actions();
        actions += m.g(u"inclusive"_q)->actions();
        if (auto next = getNextAction(actions)) {
            const int id = next->data().toInt();
            e.clearAllSubtitleSelection();
            if (e.params()->sub_tracks().contains(id))
                e.setTrackSelected(StreamSubtitle, id, true);
            else
                e.setTrackSelected(StreamInclusiveSubtitle, id, true);
        }
    });
    connect(strack[u"all"_q], &QAction::triggered, p,
            [=] () { e.setTrackSelected(StreamInclusiveSubtitle, -1, true); });
    PLUG_FLAG(strack[u"hide"_q], sub_hidden, setSubtitleHidden);
    connect(strack[u"open"_q], &QAction::triggered, p, [this] () {
        QString dir;
        if (e.mrl().isLocalFile())
            dir = _ToAbsPath(e.mrl().toLocalFile());
        auto enc = pref.sub_enc();
        const auto files = EncodingFileDialog::getOpenFileNames(
            nullptr, tr("Open Subtitle"), dir, _ToFilter(SubtitleExt), &enc);
        e.addSubtitleFiles(files, enc);
    });
    connect(strack[u"auto-load"_q], &QAction::triggered, &e, &PlayEngine::autoloadSubtitleFiles);
    connect(strack(u"reload"_q).g(), &ActionGroup::triggered, p, [=] (QAction *a) {
        const auto mib = a->data().toInt();
        // mib > 0: force, == 0: auto, < 0: current
        e.reloadSubtitleFiles(EncodingInfo::fromMib(mib), mib == 0);
    });
    connect(strack[u"clear"_q], &QAction::triggered, &e, &PlayEngine::clearSubtitleFiles);

    auto &ass = sub(u"override-ass"_q);
    PLUG_FLAG(ass[u"text"_q], sub_style_overriden, setOverrideAssTextStyle);
    PLUG_FLAG(ass[u"position"_q], sub_override_ass_position, setOverrideAssPosition);
    PLUG_FLAG(ass[u"scale"_q], sub_override_ass_scale, setOverrideAssScale);
    PLUG_ENUM(sub(u"align"_q), sub_alignment, setSubtitleAlignment);
    PLUG_STEP(sub(u"position"_q).g(u"adjust"_q), sub_position, setSubtitlePosition);
    PLUG_ENUM(sub(u"position"_q), sub_display, setSubtitleDisplay);
    PLUG_STEP(sub(u"sync"_q).g(u"step"_q), sub_sync, setSubtitleDelay);
    connect(sub(u"sync"_q).g(u"bring"_q), &ActionGroup::triggered, p, [=] (QAction *a) {
        const int time = e.captionBeginTime(a->data().toInt());
        if (time >= 0) push(e.time() - time, e.params()->sub_sync(), &PlayEngine::setSubtitleDelay);
    });
    PLUG_STEP(sub(u"scale"_q).g(), sub_scale, setSubtitleScale);

    Menu &tool = menu(u"tool"_q);
    auto &pl = tool(u"playlist"_q);
    connect(pl[u"toggle"_q], &QAction::triggered, &playlist, &PlaylistModel::toggle);
    connect(pl[u"open"_q], &QAction::triggered, p, [this] () {
        EncodingInfo enc;
        const auto filter = _ToFilter(PlaylistExt);
        qDebug() <<filter;
        const auto file = EncodingFileDialog::getOpenFileName
                (nullptr, tr("Open File"), QString(), filter, &enc);
        if (!file.isEmpty())
            playlist.open(file, enc);
    });
    connect(pl[u"save"_q], &QAction::triggered, p, [this] () {
        const auto &list = playlist.list();
        if (!list.isEmpty()) {
            auto file = _GetSaveFile(nullptr, tr("Save File"), u""_q, WritablePlaylistExt);
            if (!file.isEmpty())
                list.save(file);
        }
    });
    connect(pl[u"regenerate"_q], &QAction::triggered, p, [=] () {
        const auto mrl = e.mrl();
        const auto pl = generatePlaylist(mrl);
        if (!pl.isEmpty()) {
            playlist.setList(pl);
            playlist.setLoaded(mrl);
        }
    });
    connect(pl[u"clear"_q], &QAction::triggered, p, [=] () { playlist.clear(); });
    connect(pl[u"append-file"_q], &QAction::triggered, p, [this] () {
        const auto files = _GetOpenFiles(nullptr, tr("Open File"), MediaExt | PlaylistExt);
        Playlist list;
        for (int i=0; i < files.size(); ++i) {
            const Mrl mrl(files[i]);
            if (_IsSuffixOf(PlaylistExt, mrl.suffix())) {
                Playlist pl;
                if (pl.load(files[i]))
                    list += pl;
            } else
                list.push_back(mrl);
        }
        playlist.append(list);
    });
    connect(pl[u"append-folder"_q], &QAction::triggered, p, [this] () {
        auto dlg = dialog<OpenMediaFolderDialog>();
//        dlg->setFolder(dir);
        if (dlg->exec())
            playlist.append(dlg->playlist());
    });
    connect(pl[u"append-url"_q], &QAction::triggered, p, [this] () {
        auto dlg = dialog<UrlDialog>();
        if (dlg->exec()) {
            const Mrl mrl = dlg->url().toString();
            if (dlg->isPlaylist()) {
                Playlist list;
                list.load(mrl, dlg->encoding());
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
    connect(hm[u"clear"_q], &QAction::triggered, p, [=] () {
        if (MBox::ask(nullptr, tr("Clear History"), tr("Do you really want clear playback history?"),
                      {BBox::Yes, BBox::Cancel}, BBox::Cancel) == BBox::Yes)
            history.clear();
    });

    connect(tool[u"playinfo"_q], &QAction::triggered, p, [=] () {
        auto toggleTool = [this] (const char *name, bool &visible) {
            visible = !visible;
            if (auto item = findItem<QObject>(_L(name)))
                item->setProperty("show", visible);
        };
        toggleTool("playinfo", as.playinfo_visible);
    });
    connect(tool[u"log"_q], &QAction::triggered, logViewer.data(), &LogViewer::show);
    connect(tool[u"subtitle"_q], &QAction::triggered, p, [this] () {
        if (!sview) {
            sview = dialog<SubtitleViewer>();
            sview->setCurrentTime(e.lastSubtitleUpdatedTime());
            sview->setSeekFunc([=] (int t) { e.seek(t); });
            connect(&e, &PlayEngine::subtitleSelectionChanged, sview.data(), [=] () {
                if (sview->isVisible())
                    sview->setComponents(e.subtitleSelection());
            });
            connect(&e, &PlayEngine::subtitleUpdated, sview.data(), &SubtitleViewer::setCurrentTime);
        }
        if (!sview->isVisible())
            sview->setComponents(e.subtitleSelection());
        sview->setVisible(!sview->isVisible());
    });
    connect(tool[u"pref"_q], &QAction::triggered, p, [this] () {
        if (!prefDlg) {
            prefDlg = dialog<PrefDialog>();
            prefDlg->setAudioDeviceList(e.audioDeviceList());
            connect(prefDlg.data(), &PrefDialog::applyRequested, p,
                    [this] { prefDlg->get(&pref); applyPref(); });
        }
        if (!prefDlg->isVisible()) {
            prefDlg->set(&pref);
            prefDlg->show();
        }
    });
    connect(tool[u"associate-files"_q], &QAction::triggered,
            p, [=] () { dialog<FileAssocDialog>()->exec(); });
    connect(tool[u"find-subtitle"_q], &QAction::triggered, p, [this] () {
        if (!subFindDlg) {
            subFindDlg = dialog<SubtitleFindDialog>();
            subFindDlg->setOptions(pref.preserve_downloaded_subtitles(),
                                   pref.preserve_file_name_format(),
                                   pref.preserve_fallback_folder());
            subFindDlg->setLoadFunc([=] (const QString &fileName) {
                e.addSubtitleFiles(QStringList(fileName), pref.sub_enc());
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
            if (MBox::warn(nullptr, tr("Auto-shutdown"),
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
    connect(&playlist, &PlaylistModel::finished, p, [=, &tool] () {
        if (tool[u"auto-exit"_q]->isChecked())     p->exit();
        if (tool[u"auto-shutdown"_q]->isChecked()) OS::shutdown();
    });

    Menu &win = menu(u"window"_q);
    plugAppEnumChild(win, "win_stays_on_top", &AppState::winStaysOnTopChanged);
    connect(&as, &AppState::winStaysOnTopChanged, p, [=] () { updateStaysOnTop(); });
    connect(win[u"frameless"_q], &QAction::triggered, p, [=] (bool on) {
        adapter->setFrameless(on);
        if (_Change(as.win_frameless, on))
            emit p->framelessChanged(on);
    });

    connect(win.g(u"size"_q), &ActionGroup::triggered, p, [=] (QAction *a) {
        if (p->isFullScreen())
            p->setFullScreen(false);
        if (p->adapter()->state() == Qt::WindowMaximized)
            p->adapter()->showNormal();
        setVideoSize(videoSize(a->data().value<WindowSize>()));
    });
    connect(win[u"toggle-fs"_q], &QAction::triggered, p,
            [=] () { p->setFullScreen(!p->isFullScreen()); });
    connect(win[u"enter-fs"_q], &QAction::triggered, p, [=] () { p->setFullScreen(true); });
    connect(win[u"exit-fs"_q], &QAction::triggered, p, [=] () { p->setFullScreen(false); });

    connect(win[u"minimize"_q], &QAction::triggered, adapter, &OS::WindowAdapter::showMinimized);
    connect(win[u"maximize"_q], &QAction::triggered, adapter, &OS::WindowAdapter::showMaximized);
    connect(win[u"close"_q], &QAction::triggered, p, [=] () { menu.hide(); p->close(); });

    Menu &help = menu(u"help"_q);
    connect(help[u"about"_q], &QAction::triggered,
            p, [=] () { auto dlg = dialog<AboutDialog>(); dlg->exec(); });
    connect(menu[u"context-menu"_q], &QAction::triggered,
            p, [=] () { contextMenu.exec(QCursor::pos()); });
    connect(menu[u"exit"_q], &QAction::triggered, p, &MainWindow::exit);

#define PLUG_EC(mm, ec, EC, msg) \
    { \
        auto &m = mm; \
        connect(&e, &PlayEngine::ec##sChanged, p, [=, &m] () { \
            const auto items = e.ec##s(); \
            m.setEnabled(!items.isEmpty()); \
            m.g()->clear(); \
            for (auto item : items) { \
                auto act = m.addActionNoKey(item->name(), true); \
                act->setData(item->number()); \
            } \
        }); \
        connect(m.g(), &ActionGroup::triggered, p, [=] (QAction *a) { \
            a->setChecked(true); \
            e.seek##EC(a->data().toInt()); \
            showMessage(msg, a->text()); \
        }); \
        connect(&e, &PlayEngine::ec##Changed, p, \
                [=, &m] () { m.g()->setChecked(e.ec()->number()); }); \
    }
    PLUG_EC(play(u"title"_q), edition, Edition, tr("Current Title/Edition"));
    PLUG_EC(play(u"chapter"_q), chapter, Chapter, tr("Current Chapter"));

#define PLUG_UR(ur, UR) { \
    auto a = tool[u"" #ur ""_q]; \
    connect(&undo, &QUndoStack::can##UR##Changed, a, &QAction::setEnabled); \
    connect(a, &QAction::triggered, &undo, &QUndoStack::ur); \
    a->setEnabled(undo.can##UR()); }
    PLUG_UR(undo, Undo); PLUG_UR(redo, Redo);
}

auto MainWindow::Data::deleteDialogs() -> void
{
    sview.clear();
    prefDlg.clear();
    subFindDlg.clear();
    snapshot.clear();
    logViewer.clear();
    eq.clear();
    color.clear();
    encoder.clear();
}

