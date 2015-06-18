#include "playengine_p.hpp"
#include <QQmlEngine>
#include <QTextCodec>

template<class T>
SIA findEnum(const QString &mpv) -> T
{
    auto items = EnumInfo<T>::items();
    for (auto item : items) {
        if (mpv == item.data.property)
            return item.value;
    }
    return EnumInfo<T>::default_();
}

class OptionList {
public:
    OptionList(char join = ',')
        : m_join(join) { }
    auto add(const QByteArray &key, const QByteArray &value,
             bool quote = false) -> void
    {
        if (!m_data.isEmpty())
            m_data.append(m_join);
        m_data.append(key);
        m_data.append('=');
        if (quote)
            m_data.append('"');
        m_data.append(value);
        if (quote)
            m_data.append('"');
    }
    auto add(const QByteArray &key, void *value) -> void
        { add(key, address_cast<QByteArray>(value)); }
    auto add(const QByteArray &key, double value) -> void
        { add(key, QByteArray::number(value)); }
    auto add(const QByteArray &key, int value) -> void
        { add(key, QByteArray::number(value)); }
    auto add(const QByteArray &key, bool value) -> void
        { add(key, value ? "yes"_b : "no"_b); }
    auto addRaw(const QByteArray &key, const QByteArray &data) -> void
        { add(key, '%' + QByteArray::number(data.length()) + '%' + data); }
    auto get() const -> const QByteArray& { return m_data; }
    auto data() const -> const char* { return m_data.data(); }
private:
    QByteArray m_data;
    char m_join;
};

PlayEngine::Data::Data(PlayEngine *engine)
    : p(engine) { }

auto PlayEngine::Data::af(const MrlState *s) const -> QByteArray
{
    OptionList af(':');
    af.add("dummy:address"_b, ac);
    af.add("use_scaler"_b, (int)s->audio_tempo_scaler());
    af.add("use_normalizer"_b, (int)s->audio_volume_normalizer());
    af.add("layout"_b, (int)s->audio_channel_layout());
    return af.get();
}

auto PlayEngine::Data::vf(const MrlState *s) const -> QByteArray
{
    OptionList vf(':');
    vf.add("noformat:address"_b, vp);
    vf.add("swdec_deint"_b, s->d->deint.swdec.toString().toLatin1());
    vf.add("hwdec_deint"_b, s->d->deint.hwdec.toString().toLatin1());
    vf.add("interpolate"_b, (int)s->video_motion_interpolation());
    vf.add("color_space"_b, (int)s->video_space());
    vf.add("color_range"_b, (int)s->video_range());
    return vf.get();
}

auto PlayEngine::Data::vo(const MrlState *s) const -> QByteArray
{
    return "opengl-cb:" + videoSubOptions(s);
}

#include "misc/json.hpp"

auto PlayEngine::Data::videoSubOptions(const MrlState *s) const -> QByteArray
{
    auto c_matrix = [s] () {
        QMatrix4x4 matrix;
        if (s->video_effects() & VideoEffect::Invert)
            matrix = QMatrix4x4(-1, 0, 0, 1,
                                0, -1, 0, 1,
                                0, 0, -1, 1,
                                0, 0,  0, 1);
        auto eq = s->video_color();
        if (s->video_effects() & VideoEffect::Gray)
            eq.setSaturation(-100);
        if (!eq.isZero())
            matrix *= eq.matrix();
        if (s->video_effects() & VideoEffect::Remap) {
            const float a = 255.0 / (235.0 - 16.0);
            const float b = -16.0 / 255.0 * a;
            matrix *= QMatrix4x4(a, 0, 0, b,
                                 0, a, 0, b,
                                 0, 0, a, b,
                                 0, 0, 0, 1);
        }
        return matrix;
    };

    static const QByteArray shader =
            "const mat4 c_matrix = mat4(__C_MATRIX__); color = c_matrix * color;";
    auto customShader = [] (const QMatrix4x4 &c_matrix) -> QByteArray {
        QByteArray mat;
        for (int c = 0; c < 4; ++c) {
            mat += "vec4(";
            for (int r = 0; r < 4; ++r) {
                mat += QByteArray::number(c_matrix(r, c), 'e');
                mat += ',';
            }
            mat[mat.size()-1] = ')';
            mat += ',';
        }
        mat.chop(1);
        auto cs = shader;
        cs.replace("__C_MATRIX__", mat);
        return '%' + QByteArray::number(cs.length()) + '%' + cs;
    };

    OptionList opts(':');
    opts.add("scale", s->d->intrpl[s->video_interpolator()].toMpvOption("scale"));
    opts.add("cscale", s->d->chroma[s->video_chroma_upscaler()].toMpvOption("cscale"));
    if (useIntrplDown)
        opts.add("dscale", s->d->intrplDown[s->video_interpolator_down()].toMpvOption("dscale"));
    opts.add("dither-depth", "auto"_b);
    opts.add("dither", _EnumData(s->video_dithering()));
    opts.add("frame-queue-size", s->video_motion_interpolation() || vp->isSkipping() ? 1 : 3);
    opts.add("frame-drop-mode", s->video_motion_interpolation() ? "block"_b : "clear"_b);
    opts.add("fancy-downscaling", s->video_hq_downscaling());
    opts.add("sigmoid-upscaling", s->video_hq_upscaling() && OGL::is16bitFramebufferFormatSupported());
    opts.add("interpolation", s->video_motion_interpolation());
    const bool rgba16 = vr->framebufferObjectFormat() == OGL::RGBA16_UNorm;
    opts.add("fbo-format", rgba16 ? "rgba16"_b : "rgba"_b);
    const auto cmat = c_matrix();
    if (!cmat.isIdentity())
        opts.add("custom-shader", customShader(cmat));
    return opts.get();
}

auto PlayEngine::Data::updateVideoScaler() -> void
{
    bool use = params.video_interpolator() != Interpolator::Bilinear;
    use |= (useIntrplDown && params.video_interpolator_down() != Interpolator::Bilinear);
    vr->setScalerEnabled(use);
}

auto PlayEngine::Data::updateVideoRendererFboFormat() -> void
{
    auto gl = _EnumData(fboFormat);
    if (gl == OGL::NoTextureFormat)
        gl = OGL::is16bitFramebufferFormatSupported() ? OGL::RGBA16_UNorm : OGL::RGBA8_UNorm;
    vr->setFramebufferObjectFormat(gl);
}

auto PlayEngine::Data::updateVideoSubOptions() -> void
{
    mutex.lock();
    auto opts = videoSubOptions(&params);
    mutex.unlock();
    mpv.tellAsync("vo_cmdline", videoSubOptions(&params));
}

auto PlayEngine::Data::loadfile(const Mrl &mrl, bool resume, const QString &sub) -> void
{
    QString file = mrl.isLocalFile() ? mrl.toLocalFile() : mrl.toString();
    if (file.isEmpty())
        return;
    OptionList opts;
    opts.add("pause"_b, p->isPaused() || hasImage);
    opts.add("resume-playback", resume);
    if (!sub.isEmpty())
        opts.add("sub-file", sub.toUtf8(), true);
    if (!mrl.name().isEmpty() && mrl.isCueTrack())
        opts.addRaw("media-title", mrl.name().toUtf8());
    mpv.tell("loadfile"_b, file.toUtf8(), "replace"_b, opts.get());
}

auto PlayEngine::Data::updateMediaName(const QString &name) -> void
{
    MediaObject::Type type = MediaObject::NoMedia;
    if (mrl.isLocalFile())
        type = MediaObject::File;
    else if (mrl.isDvd())
        type = MediaObject::Dvd;
    else if (mrl.isBluray())
        type = MediaObject::Bluray;
    else
        type = MediaObject::Url;
    info.media.setName(name.isEmpty() ? mrl.displayName() : name);
    info.media.setType(type);
}

static constexpr const auto QCI = Qt::CaseInsensitive;

auto PlayEngine::Data::onLoad() -> void
{
    auto file = mpv.get<MpvFile>("stream-open-filename");
    const auto sub = mpv.get<MpvUtf8>("file-local-options/sub-file").data;
    mpv.setAsync("file-local-options/sub-file", MpvFileList());
    t.local = localCopy();
    t.seekable = t.begin = t.duration = -1;
    t.offset = 0;
    auto local = t.local.data();

    Mrl mrl(file);
    local->set_mrl(mrl.toUnique());
    local->d->disc = mrl.isDisc();

    mutex.lock();
    auto reload = this->reload;
    this->reload = -1;
    mutex.unlock();

    bool found = false, resume = false;
    int start = -1;
    if (reload < 0) {
        local->set_resume_position(-1);
        local->set_edition(-1);
        local->set_device(QString());
        local->set_star(false);
        local->set_video_tracks(StreamList());
        local->set_audio_tracks(StreamList());
        local->set_sub_tracks(StreamList());
        local->set_sub_tracks_inclusive(StreamList());
        found = history->getState(local);
        resume = mpv.get<bool>("options/resume-playback") && this->resume;
        if (resume)
            start = local->resume_position();
    } else {
        start = reload;
        local->set_device(mrl.device());
        resume = found = true;
    }

    if (mrl.isCueTrack()) {
        const auto track = mrl.toCueTrack();
        file.data = track.file;
        t.offset = t.begin = track.start;
        mpv.setAsync("file-local-options/start", QByteArray::number(track.start * 1e-3, 'f'));
        if (track.end != -1) {
            mpv.setAsync("file-local-options/end", QByteArray::number(track.end * 1e-3, 'f'));
            t.duration = track.end - t.begin;
        }
    }

    if (file.data.startsWith("smb://"_a, QCI)) {
        auto smb = local->d->smb;
        QUrl url = smb.translate(QUrl(file));
        bool ok = false;
        for (;;) {
            if (smb.process(url) == SmbAuth::NoError) {
                file = url.toString(QUrl::FullyEncoded);
                ok = true;
                break;
            }
            _Error("Failed to access smb share: %%", smb.lastErrorString());
            if (smb.lastError() != SmbAuth::NoPermission)
                break;
            if (!smb.getNewAuthInfo()) {
                _Error("Failed to acquire new authentication for smb://.");
                break;
            }
            url.setUserName(smb.username());
            url.setPassword(smb.password());
        }
        if (ok) {
//            smb.openDir(Mrl(file.data));
        }
    }

    auto setFiles = [&] (QByteArray &&name, QByteArray &&nid,
            const StreamList &list) {
        MpvFileList files; int id = -1;
        for (auto &track : list) {
            if (!track.isExclusive())
                continue;
            if (track.isExternal())
                files.names.push_back(track.file());
            if (track.isSelected())
                id = track.id();
        }
        if (!files.names.isEmpty())
            mpv.setAsync(std::move(name), files);
        if (id != -1)
            mpv.setAsync(std::move(nid), id);
    };

    if (found && local->audio_tracks().isValid())
        setFiles("file-local-options/audio-file"_b, "file-local-options/aid"_b, local->audio_tracks());
    else {
        QMutexLocker locker(&mutex);
        mpv.setAsync("file-local-options/audio-file", autoloadFiles(StreamAudio));
    }
    QVector<SubComp> loads;
    auto loadSub = [&] (auto &&res) {
        MpvFileList files, encs;
        _R(files, loads) = res;
        if (!files.names.isEmpty()) {
            mpv.setAsync("options/subcp", assEncodings[files.names.front()].name().toLatin1());
            mpv.setAsync("file-local-options/sub-file", files);
        }
        bool sel = false;
        for (const auto &s : loads) {
            sel = s.selection();
            if (sel)
                break;
        }
        if (sel && local->d->preferExternal)
            mpv.setAsync("file-local-options/sid", "no"_b);
        else
            mpv.setAsync("file-local-options/sid", "auto"_b);
    };
    if (sub.isEmpty()) {
        if (found && local->sub_tracks().isValid()) {
            setFiles("file-local-options/sub-file"_b, "file-local-options/sid"_b, local->sub_tracks());
            loads = restoreInclusiveSubtitles(local->sub_tracks_inclusive(), EncodingInfo(), -1);
        } else {
            QMutexLocker locker(&mutex);
            loadSub(autoloadSubtitle(local));
        }
    } else {
        QMutexLocker locker(&mutex);
        loadSub(autoloadSubtitle(local, QStringList{sub}));
        for (auto &comp : loads)
            comp.selection() = true;
    }

    local->set_last_played_date_time(QDateTime::currentDateTime());
    local->set_device(mrl.device());
    local->set_mrl(mrl);

    int edition = -1;
    if (resume && mrl.isUnique() && !mrl.isImage())
        edition = local->edition();
    else
        start = -1;

    const auto deint = local->video_deinterlacing() != DeintMode::None;
    mpv.setAsync("speed", local->play_speed());
    mpv.setAsync("video-rotate", _EnumData(local->video_rotation()));

    mpv.setAsync("options/vo", vo(local));
    mpv.setAsync("options/vf", vf(local));
    mpv.setAsync("options/deinterlace", deint ? "yes"_b : "no"_b);

    mpv.setAsync("options/af", af(local));
    mpv.setAsync("options/volume", volume(local));
    mpv.setAsync("options/mute", local->audio_muted() ? "yes"_b : "no"_b);
    mpv.setAsync("options/audio-delay", local->audio_sync() * 1e-3);
    mpv.setAsync("options/audio-channels", _ChmapNameFromLayout(local->audio_channel_layout()));

    // missing options: sub_alignment, sub_display, sub_position
    mpv.setAsync("options/sub-visibility", !local->sub_hidden());
    mpv.setAsync("options/sub-delay", local->sub_sync() * 1e-3);

    const auto cache = local->d->cache.get(mrl);
    t.caching = cache.kb > 0LL;
    if (t.caching) {
        mpv.setAsync("file-local-options/cache", cache.kb);
        mpv.setAsync("file-local-options/cache-initial", local->d->cache.playback_kb(cache.kb));
        mpv.setAsync("file-local-options/cache-seek-min", local->d->cache.seeking_kb(cache.kb));
        mpv.setAsync("file-local-options/cache-secs", cache.sec);
        mpv.setAsync("file-local-options/cache-file", cache.file ? "TMP"_b : ""_b);
        mpv.setAsync("file-local-options/cache-file-size", local->d->cache.file_kb);
    } else
        mpv.setAsync("file-local-options/cache", "no"_b);

    if (file.data.startsWith("http://"_a, QCI) || file.data.startsWith("https://"_a, QCI)) {
        file = QUrl(file).toString(QUrl::FullyEncoded);
        if (file != ytResult.mrl)
            ytResult.clear();
        if (yle && yle->supports(file)) {
            if (yle->run(file)) {
                start = -1;
                file = yle->url();
            }
        } else if (ytResult.mrl == file || (youtube && youtube->run(file))) {
            mpv.setAsync("file-local-options/cookies", true);
            mpv.setAsync("file-local-options/cookies-file", MpvFile(youtube->cookies()).toMpv());
            mpv.setAsync("file-local-options/user-agent", youtube->userAgent().toUtf8());
            auto &r = ytResult;
            if (r.mrl != file)
                r = youtube->result();
            if (!r.title.isEmpty())
                mpv.setAsync("file-local-options/media-title", r.title.toUtf8());
            if (r.duration > 0)
                t.duration = r.duration;
            if (r.live)
                start = -1;
            if (r.videos.isEmpty()) {
                if (!r.url.isEmpty())
                    file = r.url;
            } else {
                int idx = -1;
                mutex.lock();
                const auto selection = r.selection;
                mutex.unlock();
                if (!selection.isEmpty()) {
                    for (int i = 0; i < r.videos.size(); ++i) {
                        if (r.videos[i].id() == selection) {
                            idx = i;
                            break;
                        }
                    }
                }
                if (idx < 0)
                    idx = std::max(0, youtube->select(r.videos));
                const auto &video = r.videos[idx];
                mutex.lock();
                r.selection = video.id();
                mutex.unlock();
                file = video.url();
                if (video.isDash() && r.audio.isValid()) {
                    mpv.setAsync("file-local-options/audio-file", r.audio.url().toUtf8());
                    mpv.setAsync("file-local-options/demuxer-lavf-o", "fflags=+ignidx"_b);
                }
            }
        }
    } else
        ytResult.clear();

    if (local->d->disc) {
        file = mrl.titleMrl(edition >= 0 ? edition : -1).toString();
        t.start = start;
    } else {
        if (edition >= 0)
            mpv.setAsync("file-local-options/edition", edition);
        if (start > 0) {
            const auto secs = QByteArray::number((start + t.offset) * 1e-3, 'f');
            mpv.setAsync("file-local-options/start", secs);
        }
        t.start = -1;
    }

    mpv.setAsync("stream-open-filename", file.toMpv());
    mpv.flush();
    _PostEvent(p, SyncMrlState, t.local, loads, ytResult);
    t.local.clear();

    mutex.lock();
    playingVideo = file.toMpv();
    mutex.unlock();
}

auto PlayEngine::Data::onUnload() -> void
{
    t.local = localCopy();
    t.local->set_name(mpv.get<MpvUtf8>("media-title").data);
    t.local->set_resume_position(time);
    t.local->set_last_played_date_time(QDateTime::currentDateTime());
    t.local->set_edition(info.edition.number());
}

auto PlayEngine::Data::hook() -> void
{
    mpv.hook("on_load", [=] () { onLoad(); });
    mpv.hook("on_unload", [=] () { onUnload(); });
}

SCA calcFrameCount(double fps, int ms) -> qint64
{
    return fps * (ms * 1e-3) + 0.5;
}

auto PlayEngine::Data::observe() -> void
{
    mpv.setObserver(p);
    mpv.observeState("pause", [=] (bool p)
        { if (p) post(Paused); else if (!mpv.get<bool>("idle")) post(Playing); });
    mpv.observeState("core-idle", [=] (bool i) { if (!i) post(Playing); });
    mpv.observeState("paused-for-cache", [=] (bool b) { post(Buffering, b); });
    mpv.observeState("seeking", [=] (bool s) { post(Seeking, s); });

    mpv.observe("cache-used", [=] () { return t.caching ? mpv.get<int>("cache-used") : 0; },
                [=] (int v) { info.cache.setUsed(v); });
    mpv.observe("cache-size", [=] () { return t.caching ? mpv.get<int>("cache-size") : 0; },
                [=] (int v) { info.cache.setSize(v); });

    mpv.observe("seekable", [=] () {
        return t.seekable >= 0 ? !!t.seekable : mpv.get<bool>("seekable");
    }, [=] (bool s) { if (_Change(seekable, s)) emit p->seekableChanged(seekable); });

    auto updateChapter = [=] (int n) {
        info.chapter.invalidate();
        for (auto chapter : info.chapters) {
            chapter->setRate(p->rate(chapter->time()));
            if (chapter->number() == n)
                info.chapter.set(chapter->m);
        }
        emit p->chapterChanged();
    };

    mpv.observeTime("avsync", avSync, [=] () { emit p->avSyncChanged(avSync); });
    mpv.observe("time-pos", [=] () {
        int ctime = 0;
        if (t.caching)
            ctime = s2ms(mpv.get<double>("demuxer-cache-time")) - t.offset;
        if (ctime != info.cache.time())
            QMetaObject::invokeMethod(&info.cache, "setTime",
                                      Qt::QueuedConnection, Q_ARG(int, ctime));
        return s2ms(mpv.get<double>("time-pos")) - t.offset;
    }, [=] (int pos) {
        if (!_Change(time, pos))
            return;
        emit p->tick(time);
        if (_Change(time_s, time/1000))
            emit p->time_sChanged();
        sr->render(time);
        info.video.setFrameNumber(calcFrameCount(info.video.decoder()->fps(), time - begin));
    });
    mpv.observe("time-start", [=] () {
        return (t.begin < 0 ? s2ms(mpv.get<double>("time-start")) : t.begin) - t.offset;
    }, [=] (int ms) {
        if (!_Change(begin, ms))
            return;
        emit p->beginChanged(begin);
        if (_Change(begin_s, begin/1000))
            emit p->begin_sChanged();
        updateChapter(mpv.get<int>("chapter"));
    });

    auto length = [=] () {
        if (t.duration >= 0)
            return t.duration;
        const int len = s2ms(mpv.get<double>("length"));
        if (t.begin >= 0)
            return t.duration = s2ms(mpv.get<double>("time-start")) + len - t.begin;
        return len;
    };
    mpv.observe("length", [=] () { return length(); }, [=] (int ms) {
        if (!_Change(duration, ms))
            return;
        emit p->durationChanged(duration);
        if (_Change(duration_s, duration/1000))
            emit p->duration_sChanged();
        updateChapter(mpv.get<int>("chapter"));
        info.video.setFrameCount(calcFrameCount(info.video.decoder()->fps(), duration));
    });

    mpv.observe("chapter-list", [=] () {
        const auto array = mpv.get<QVariant>("chapter-list").toList();
        QVector<ChapterData> data(array.size());
        for (int i=0; i<array.size(); ++i) {
            const auto map = array[i].toMap();
            data[i].number = i;
            data[i].time = s2ms(map[u"time"_q].toDouble()) - t.offset;
            data[i].name = map[u"title"_q].toString();
            if (data[i].name.isEmpty())
                data[i].name = _MSecToString(data[i].time, u"hh:mm:ss.zzz"_q);
        }
        return data;
    }, [=] (auto &&data) {
        for (auto chapter : info.chapters)
            QQmlEngine::setObjectOwnership(chapter, QQmlEngine::JavaScriptOwnership);
        info.chapters.resize(data.size());
        for (int i = 0; i < data.size(); ++i) {
            info.chapters[i] = new ChapterObject;
            info.chapters[i]->set(data[i]);
        }
        emit p->chaptersChanged();
        updateChapter(mpv.get<int>("chapter"));
    });
    mpv.observe("chapter", updateChapter);
    mpv.observe("track-list", [=] () {
        return toTracks(mpv.get<QVariant>("track-list"));
    }, [=] (auto &&strms) {
        params.set_video_tracks(strms[StreamVideo]);
        params.set_audio_tracks(strms[StreamAudio]);
        params.set_sub_tracks(strms[StreamSubtitle]);

        auto audioOnly = !strms[StreamAudio].isEmpty();
        if (audioOnly && !strms[StreamVideo].isEmpty()) {
            for (auto &track : _C(strms[StreamVideo])) {
                if (!track.isAlbumArt()) {
                    audioOnly = false;
                    break;
                }
            }
        }
        if (_Change(this->audioOnly, audioOnly))
            emit p->audioOnlyChanged(audioOnly);
        if (strms[StreamSubtitle].isEmpty())
            vr->setOsdVisible(false);
    });

    for (auto type : streamTypes)
        mpv.observe(streams[type].pid, [=] (QVariant &&var) {
            int current = -1;
            if (var.type() == QVariant::Int)
                params.select(type, current = var.toInt());
            else if (var.type() == QVariant::String) {
                const auto id = var.toString();
                if (id == "no"_a)
                    params.select(type, -1);
                else if (id != "auto"_a)
                    _Error("'%%' is not a valid id for stream id.", var.toString());
            }
            if (type == StreamSubtitle)
                vr->setOsdVisible(current > 0);
        });
    mpv.observe("metadata", [=] () {
        const auto map = mpv.get<QVariant>("metadata").toMap();
        MetaData metaData;
        metaData.m_title = map[u"title"_q].toString();
        metaData.m_artist = map[u"artist"_q].toString();
        metaData.m_genre = map[u"genre"_q].toString();
        metaData.m_date = map[u"date"_q].toString();
        metaData.m_mrl = params.mrl();
        metaData.m_duration = length();
        return metaData;
    }, [=] (auto &&md) {
        if (_Change(metaData, md))
            emit p->metaDataChanged();
    });
    mpv.observe("media-title", [=] (MpvUtf8 &&t) { updateMediaName(t); });

    mpv.observe("video-codec", [=] (MpvLatin1 &&c) { info.video.codec()->parse(c); });
    mpv.observe("fps", [=] (double fps) {
        info.video.decoder()->setFps(fps);
        info.video.filter()->setFps(fps);
        sr->setFPS(fps);
        info.video.setFrameCount(calcFrameCount(fps, duration));
        info.video.setFrameNumber(calcFrameCount(fps, time - begin));
    });
    mpv.observe("width", [=] (int w) {
        auto input = info.video.decoder();
        input->setWidth(w);
        input->setBppSize(input->size());
    });
    mpv.observe("height", [=] (int h) {
        auto input = info.video.decoder();
        input->setHeight(h);
        input->setBppSize(input->size());
    });
    mpv.observe("video-bitrate", [=] (int bps) { info.video.decoder()->setBitrate(bps); });
    mpv.observe("video-format", [=] (MpvLatin1 &&f) { info.video.decoder()->setType(f); });
    QRegularExpression rx(uR"(Video decoder: ([^\n]*))"_q);
    auto filterInput = [=] (const char *name) -> QString {
        auto m = rx.match(mpv.get_osd(name));
        if (!m.hasMatch() || m.capturedRef(1) == "unknown"_a)
            return u"Autoselect"_q;
        return m.captured(1);
    };

    auto setParams = [] (VideoFormatObject *info, const QVariantMap &p,
                         const QString &wkey, const QString &hkey) {
        const auto type = p[u"pixelformat"_q].toString();
        const auto w = p[wkey].toInt(), h = p[hkey].toInt();
        info->setType(type);
        info->setSize({w, h});
        info->setBppSize({p[u"w"_q].toInt(), p[u"h"_q].toInt()},
                         p[u"average-bpp"_q].toInt());
        info->setDepth(p[u"plane-depth"_q].toInt());
        info->setRotation(p[u"rotate"_q].toInt());
    };
    mpv.observe("video-params", [=] (QVariant &&var) {
        const auto params = var.toMap();
        auto &video = info.video;
        auto info = video.filter();
        setParams(info, params, u"w"_q, u"h"_q);
    });
    mpv.observe("video-out-params", [=] (QVariant &&var) {
        const auto params = var.toMap();
        auto info = this->info.video.output();
        setParams(info, params, u"dw"_q, u"dh"_q);
    });

    mpv.observe("audio-codec", [=] (MpvLatin1 &&c) { info.audio.codec()->parse(c); });
    mpv.observe("audio-format", [=] (MpvLatin1 &&f) { info.audio.decoder()->setType(f); });
    mpv.observe("audio-bitrate", [=] (int bps) { info.audio.decoder()->setBitrate(bps); });
    mpv.observe("audio-samplerate", [=] (int s) { info.audio.decoder()->setSampleRate(s, false); });
    mpv.observe("audio-channels", [=] (int n)
        { info.audio.decoder()->setChannels(QString::number(n) % "ch"_a, n); });
    mpv.observe("audio-device", [=] (MpvLatin1 &&d) { info.audio.setDevice(d); });
    mpv.observe("current-ao", [=] (MpvLatin1 &&ao) { info.audio.setDriver(ao); });

    mpv.observe("disc-mouse-on-button", [=] (bool on) { mouseOnButton = on; });
}

auto PlayEngine::Data::request() -> void
{
    mpv.request(MPV_EVENT_START_FILE, [=] () {
        _PostEvent(p, PreparePlayback);
        post(mpv.get<bool>("pause") ? Paused : Playing);
        post(Loading, true);
    });
    mpv.request(MPV_EVENT_FILE_LOADED, [=] () {
        post(mpv.get<bool>("pause") ? Paused : Playing);
        post(Loading, false);
        const auto disc = params.mrl().isDisc();
        if (t.start > 0) {
            mpv.tellAsync("seek", t.start * 1e-3, "absolute"_b);
            t.start = -1;
        }
        const char *listprop = disc ? "disc-titles" : "editions";
        const char *itemprop = disc ? "disc-title"  : "edition";
        const int list = mpv.get<int>(listprop);
        QVector<EditionData> editions(list);
        auto name = disc ? tr("Title %1") : tr("Edition %1");
        for (int i = 0; i < list; ++i) {
            editions[i].number = i;
            editions[i].name = name.arg(i+1);
        }
        EditionData edition;
        if (list > 0) {
            const int item = mpv.get<int>(itemprop);
            if (0 <= item && item < list)
                edition = editions[item];
        }

        auto strs = toTracks(mpv.get<QVariant>("track-list"));
        auto select = [&] (StreamType type) {
            for (auto &p : streams[type].priority) {
                const QRegEx rx(p);
                for (auto &str : strs[type]) {
                    auto m = rx.match(str.language());
                    if (m.hasMatch()) {
                        str.m_selected = true;
                        mpv.setAsync(streams[type].pid, str.id());
                    }
                }
            }
        };
        select(StreamAudio);
        select(StreamSubtitle);
        mpv.flush();
        _PostEvent(p, StartPlayback, editions, edition);
        QByteArray path;
        if (!ytResult.videos.isEmpty()) {
            int idx = -1;
            for (int i = ytResult.videos.size() - 1; i >= 0; --i) {
                const auto &video = ytResult.videos[i];
                if (video.height() >= 200 && video.extension() == "webm"_a) {
                    idx = i;
                    break;
                }
            }
            if (idx < 0) {
                for (int i = ytResult.videos.size() - 1; i >= 0; --i) {
                    if (ytResult.videos[i].height() >= 200) {
                        idx = i;
                        break;
                    }
                }
            }
            if (idx < 0)
                idx = ytResult.videos.size() - 1;
            path = ytResult.videos[idx].url().toUtf8();
        } else {
            auto file = mpv.get<MpvFile>("stream-open-filename");
            if (disc && edition.number >= 0)
                file = Mrl(file).titleMrl(edition.number).toString();
            path = file.toMpv();
        }
        preview->load(path);
    });
    mpv.request(MPV_EVENT_END_FILE, [=] (mpv_event *e) {
        preview->unload();
        post(Loading, false);
        auto ev = static_cast<mpv_event_end_file*>(e->data);
        _PostEvent(p, EndPlayback, t.local, ev->reason, ev->error);
        t.local.clear();
    });
    mpv.request(MPV_EVENT_PLAYBACK_RESTART, [=] () {
        _PostEvent(p, NotifySeek);
    });
}

auto PlayEngine::Data::process(QEvent *event) -> void
{
    if (mpv.process(event))
        return;
    switch ((int)event->type()) {
    case StateChange:
        updateState(_GetData<PlayEngine::State>(event));
        break;
    case WaitingChange: {
        bool set = false; Waitings waitings = NoWaiting;
        _TakeData(event, waitings, set);
        setWaitings(waitings, set);
        break;
    } case PreparePlayback: {
        break;
    } case StartPlayback: {
        clearTimings();
        QVector<EditionData> editions; EditionData edition;
        _TakeData(event, editions, edition);
        qDeleteAll(info.editions);
        info.editions.resize(editions.size());
        for (int i = 0; i < info.editions.size(); ++i) {
            info.editions[i] = new EditionObject;
            info.editions[i]->set(editions[i]);
        }
        info.edition.set(edition);
        emit p->editionsChanged();
        emit p->editionChanged();
        emit p->started(params.mrl());
        if (params.set_name(mpv.get<MpvUtf8>("media-title").data))
            history->update(&params, u"name"_q, false);
        history->update();
        break;
    } case EndPlayback: {
        QSharedPointer<MrlState> last; int reason, error;
        _TakeData(event, last, reason, error);
        Q_ASSERT(last.data());
        auto state = Stopped;
        bool eof = false;
        switch ((mpv_end_file_reason)reason) {
        case MPV_END_FILE_REASON_EOF:
        case MPV_END_FILE_REASON_REDIRECT:
            last->set_resume_position(-1);
            eof = true;
            _Info("Playback reached end-of-file");
            break;
        case MPV_END_FILE_REASON_QUIT:
        case MPV_END_FILE_REASON_STOP:
            _Info("Playback has been terminated by request");
            break;
        case MPV_END_FILE_REASON_ERROR:
            _Error("Playback has been terminated by error: %%", mpv_error_string(error));
            state = Error;
            break;
        }
        updateState(state);
        history->update(last.data(), false);
        emit p->finished(last->mrl(), eof);
        break;
    } case NotifySeek:
        emit p->sought();
        break;
    case SyncMrlState: {
        QSharedPointer<MrlState> ms;
        QVector<SubComp> loads;
        YouTubeDL::Result ytr;
        _TakeData(event, ms, loads, ytr);
        emit p->beginSyncMrlState();
        params.m_mutex = nullptr;
        sr->setComponents(loads);
        mutex.lock();
        params.copyFrom(ms.data());
        params.set_sub_tracks_inclusive(sr->toTrackList());
        mutex.unlock();
        params.m_mutex = &mutex;
        emit p->endSyncMrlState();
        history->update(&params, false);

        qDeleteAll(info.streamings);
        info.streamings.clear();
        for (auto &fmt : ytr.videos) {
            if (ytr.selection == fmt.id())
                info.streaming.set(fmt.id(), fmt.description());
            info.streamings.push_back(new StreamingFormatObject(fmt.id(), fmt.description()));
        }
        emit p->streamingFormatsChanged();
        emit p->streamingFormatChanged();
        break;
    } default:
        break;
    }
}

auto PlayEngine::Data::takeSnapshot() -> void
{
    p->clearSnapshots();
    const auto size = displaySize();
    if (size.isEmpty()) {
        emit p->snapshotTaken();
        return;
    }
    Fbo frame(size), osd(size);
    mpv.render(&frame, &osd, QMargins());
    ss.frame = frame.texture().toImage(QImage::Format_ARGB32);
    ss.osd = osd.texture().toImage(QImage::Format_ARGB32_Premultiplied);
    ss.time = mpv.get<double>("time-pos") * 1e3;
    emit p->snapshotTaken();
}

auto PlayEngine::Data::renderVideoFrame(Fbo *frame, Fbo *osd, const QMargins &m) -> void
{
    info.delayed = mpv.render(frame, osd, m);
    frames.measure.push(++frames.drawn);

    _Trace("PlayEngine::Data::renderVideoFrame(): "
           "render queued frame(%%), avgfps: %%",
           frame->size(), info.video.output()->fps());

    if (ss.take) {
        takeSnapshot();
        ss.take = false;
    }
}

auto PlayEngine::Data::toTracks(const QVariant &var) -> QVector<StreamList>
{
    QVector<StreamList> streams(3);
    streams[StreamVideo] = { StreamVideo };
    streams[StreamAudio] = { StreamAudio };
    streams[StreamSubtitle] = { StreamSubtitle };
    auto list = var.toList();
    for (auto &var : list) {
        auto track = StreamTrack::fromMpvData(var);
        streams[track.type()].insert(track);
    }
    auto &subs = streams[StreamSubtitle];
    if (!subs.isEmpty()) {
        QMutexLocker locker(&mutex);
        for (auto &track : subs) {
            if (track.isExternal())
                track.m_encoding = assEncodings.take(track.file());
        }
    }
    return streams;
}

auto PlayEngine::Data::restoreInclusiveSubtitles(const StreamList &tracks, const EncodingInfo &enc, bool detect) -> QVector<SubComp>
{
    Q_ASSERT(tracks.type() == StreamInclusiveSubtitle);
    QVector<SubComp> ret;
    QMap<QString, QMap<QString, SubComp>> subMap;
    for (auto &track : tracks) {
        auto it = subMap.find(track.file());
        if (it == subMap.end()) {
            it = subMap.insert(track.file(), QMap<QString, SubComp>());
            Subtitle sub;
            if (!sub.load(track.file(), encoding(track, enc, detect)))
                continue;
            for (int i = 0; i < sub.size(); ++i)
                it->insert(sub[i].language(), sub[i]);
        }
        auto iit = it->find(track.language());
        if (iit != it->end()) {
            iit->selection() = track.isSelected();
            ret.push_back(*iit);
        }
    }
    return ret;
}

auto PlayEngine::Data::autoloadFiles(StreamType type) -> MpvFileList
{
    auto &a = streams[type].autoloader;
    if (!a.enabled)
        return MpvFileList();
    return a.autoload(mrl, streams[type].ext);
}

auto PlayEngine::Data::autoselect(const MrlState *s, QVector<SubComp> &loads) -> void
{
    QVector<int> selected;
    QSet<QString> langSet;
    const QFileInfo file(mrl.toLocalFile());
    const QString base = file.completeBaseName();

    for (int i = 0; i<loads.size(); ++i) {
        bool select = false;
        switch (s->d->autoselectMode) {
        case AutoselectMode::Matched: {
            const QFileInfo info(loads[i].fileName());
            select = info.completeBaseName() == base;
            break;
        } case AutoselectMode::EachLanguage: {
//                  const QString lang = loaded[i].m_comp.language().id();
            const auto lang = loads[i].language();
            if ((select = (!langSet.contains(lang))))
                langSet.insert(lang);
            break;
        }  case AutoselectMode::All:
            select = true;
            break;
        default:
            break;
        }
        if (select)
            selected.append(i);
    }
    if (s->d->autoselectMode == AutoselectMode::Matched
            && !selected.isEmpty() && !s->d->autoselectExt.isEmpty()) {
        for (int i=0; i<selected.size(); ++i) {
            const auto fileName = loads[selected[i]].fileName();
            const auto suffix = QFileInfo(fileName).suffix();
            if (s->d->autoselectExt == suffix.toLower()) {
                const int idx = selected[i];
                selected.clear();
                selected.append(idx);
                break;
            }
        }
    }
    for (int i=0; i<selected.size(); ++i)
        loads[selected[i]].selection() = true;
}

auto PlayEngine::Data::autoloadSubtitle(const MrlState *s, const MpvFileList &subs)
-> T<MpvFileList, QVector<SubComp>>
{
    MpvFileList files;
    QVector<SubComp> loads;
    for (auto &file : subs.names) {
       Subtitle sub;
       const auto enc = EncodingInfo::detect(EncodingInfo::Subtitle, file);
       if (sub.load(file, enc)) {
           for (int i = 0; i < sub.size(); ++i)
               loads.push_back(sub[i]);
       } else {
           files.names.push_back(file);
           assEncodings[file] = enc;
       }
    }
    autoselect(s, loads);
    return _T(files, loads);
}

auto PlayEngine::Data::autoloadSubtitle(const MrlState *s) -> T<MpvFileList, QVector<SubComp>>
{
    return autoloadSubtitle(s, autoloadFiles(StreamSubtitle));
}

auto PlayEngine::Data::localCopy() -> QSharedPointer<MrlState>
{
    auto s = new MrlState;
    s->blockSignals(true);
    mutex.lock();
    s->copyFrom(&params);
    mutex.unlock();
    return QSharedPointer<MrlState>(s);
}

auto PlayEngine::Data::updateState(State s) -> void
{
    const auto prev = state;
    if (_Change(state, s)) {
        emit p->stateChanged(state);
        auto check = [&] (State s)
            { return !!(state & s) != !!(prev & s); };
        if (check(Paused))
            emit p->pausedChanged();
        if (check(Playing))
            emit p->playingChanged();
        if (check(Stopped))
            emit p->stoppedChanged();
        if (check(Running))
            emit p->runningChanged();
        if (p->isRunning())
            info.frameTimer.start();
        else
            info.frameTimer.stop();
    }
}

auto PlayEngine::Data::setWaitings(Waitings w, bool set) -> void
{
    auto old = p->waiting();
    if (set)
        waitings |= w;
    else
        waitings &= ~w;
    auto new_ = p->waiting();
    if (old != new_)
        emit p->waitingChanged(new_);
}

auto PlayEngine::Data::clearTimings() -> void
{
    frames.measure.reset();
    info.video.setDroppedFrames(0);
    info.video.setDelayedFrames(0);
    info.video.output()->setFps(0);
    frames.drawn = 0;
}

auto PlayEngine::Data::sub_add(const QString &file, const EncodingInfo &enc, bool select) -> void
{
    mpv.setAsync("options/subcp", enc.name().toLatin1());
    mpv.tellAsync("sub_add", MpvFile(file), select ? "select"_b : "auto"_b);
    mutex.lock();
    assEncodings[file] = enc;
    mutex.unlock();
}

auto PlayEngine::Data::setSubtitleFiles(const QVector<SubtitleWithEncoding> &subs) -> void
{
    p->clearSubtitleFiles();
    addSubtitleFiles(subs);
}


auto PlayEngine::Data::addSubtitleFiles(const QVector<SubtitleWithEncoding> &subs) -> void
{
    if (subs.isEmpty())
        return;
    QVector<SubComp> loaded;
    for (auto &s : subs) {
        const auto enc = EncodingInfo::detect(EncodingInfo::Subtitle, s.encoding, s.file);
        Subtitle sub;
        if (sub.load(s.file, enc)) {
            for (int i = 0; i < sub.size(); ++i) {
                loaded.push_back(sub[i]);
                loaded.back().selection() = true;
            }
        } else {
            mpv.setAsync("options/subcp", enc.name().toLatin1());
            mpv.tellAsync("sub_add", MpvFile(s.file), "auto"_b);
            mutex.lock();
            assEncodings[s.file] = enc;
            mutex.unlock();
        }
    }
    sr->addComponents(loaded);
    syncInclusiveSubtitles();
}

auto PlayEngine::Data::updateSubtitleStyle() -> void
{
    auto style = subStyle;
    style.font.size = qBound(0.0, style.font.size * (1.0 + params.sub_scale()), 1.0);
    sr->setStyle(style);

    const auto font = subStyle.font;
    mpv.setAsync("options/sub-text-color", font.color.name(QColor::HexArgb).toLatin1());
    QStringList fontStyles;
    if (font.bold())
        fontStyles.append(u"Bold"_q);
    if (font.italic())
        fontStyles.append(u"Italic"_q);
    QString family = font.family();
    if (!fontStyles.isEmpty())
        family += ":style="_a % fontStyles.join(' '_q);
    const double factor = font.size * 720.0;
    mpv.setAsync("options/sub-text-font", family.toUtf8());
    mpv.setAsync("options/sub-text-font-size", factor);
    const auto &outline = style.outline;
    const auto scaled = [factor] (double v)
        { return qBound(0., v*factor, 10.); };
    const auto color = [] (const QColor &color)
        { return color.name(QColor::HexArgb).toLatin1(); };
    if (outline.enabled) {
        mpv.setAsync("options/sub-text-border-size", scaled(outline.width));
        mpv.setAsync("options/sub-text-border-color", color(outline.color));
    } else
        mpv.setAsync("options/sub-text-border-size", 0.0);
    const auto &bbox = style.bbox;
    if (bbox.enabled)
        mpv.setAsync("options/sub-text-back-color", color(bbox.color));
    else
        mpv.setAsync("options/sub-text-back-color", color(Qt::transparent));
    auto norm = [] (const QPointF &p) { return sqrt(p.x()*p.x() + p.y()*p.y()); };
    const auto &shadow = style.shadow;
    if (shadow.enabled) {
        mpv.setAsync("options/sub-text-shadow-color", color(shadow.color));
        mpv.setAsync("options/sub-text-shadow-offset", scaled(norm(shadow.offset)));
    } else {
        mpv.setAsync("options/sub-text-shadow-color", color(Qt::transparent));
        mpv.setAsync("options/sub-text-shadow-offset", 0.0);
    }
}

auto PlayEngine::Data::resync(bool force) -> void
{
    if (!p->isRunning() || !filterResync)
        return;
    if (!force && !params.audio_volume_normalizer())
        return;
    mpv.tellAsync("seek", 0.0, "relative"_b);
}

auto PlayEngine::Data::volume(const MrlState *s) const -> double
{
    auto x = s->audio_volume();
    if (volumeScale > 0 && x > 1e-8) {
        const auto a = M_LN10 * volumeScale / 20.0;
        const auto b = exp(-a);
        x = std::min(b * exp(a * x), 1.0);
    }
    return x * 100 * s->audio_amplifier();
}

