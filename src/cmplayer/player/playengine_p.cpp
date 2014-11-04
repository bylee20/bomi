#include "playengine_p.hpp"

auto reg_play_engine() -> void
{
    qRegisterMetaType<PlayEngine::State>("State");
    qRegisterMetaType<Mrl>("Mrl");
    qRegisterMetaType<VideoFormat>("VideoFormat");
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<StreamList>("StreamList");
    qRegisterMetaType<AudioFormat>("AudioFormat");
    qmlRegisterType<ChapterInfoObject>();
    qmlRegisterType<AudioTrackInfoObject>();
    qmlRegisterType<SubtitleTrackInfoObject>();
    qmlRegisterType<AvInfoObject>();
    qmlRegisterType<AvIoFormat>();
    qmlRegisterType<MediaInfoObject>();
    qmlRegisterType<PlayEngine>("CMPlayer", 1, 0, "Engine");
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
    auto get() const -> const QByteArray& { return m_data; }
    auto data() const -> const char* { return m_data.data(); }
private:
    QByteArray m_data;
    char m_join;
};

PlayEngine::Data::Data(PlayEngine *engine)
    : p(engine) { }

auto PlayEngine::Data::af() const -> QByteArray
{
    OptionList af(':');
    af.add("dummy:address"_b, audio);
    af.add("use_scaler"_b, (int)tempoScaler);
    af.add("layout"_b, (int)layout);
    return af.get();
}
auto PlayEngine::Data::vf() const -> QByteArray
{
    OptionList vf(':');
    vf.add("noformat:address"_b, filter);
    vf.add("swdec_deint"_b, deint_swdec.toString().toLatin1());
    vf.add("hwdec_deint"_b, deint_hwdec.toString().toLatin1());
    return vf.get();
}

auto PlayEngine::Data::vo() const -> QByteArray
{
    OptionList vo(':');
    vo.add("null:address"_b, video);
    return vo.get();
}

auto PlayEngine::Data::tellmpv(const QByteArray &cmd) -> void
{
    if (handle)
        check(mpv_command_string(handle, cmd.constData()),
              "Cannaot execute: %%", cmd);
}

auto PlayEngine::Data::tellmpv_async(const QByteArray &cmd,
                   std::initializer_list<QByteArray> &&list) -> void
{
    QVector<const char*> args(list.size()+2, nullptr);
    auto it = args.begin();
    *it++ = cmd.constData();
    for (auto &one : list)
        *it++ = one.constData();
    if (handle)
        check(mpv_command_async(handle, 0, args.data()),
              "Cannot execute: %%", cmd);
}

auto PlayEngine::Data::tellmpv(const QByteArray &cmd,
             std::initializer_list<QByteArray> &&list) -> void
{
    QVector<const char*> args(list.size()+2, nullptr);
    auto it = args.begin();
    *it++ = cmd.constData();
    for (auto &one : list)
        *it++ = one.constData();
    if (handle)
        check(mpv_command(handle, args.data()), "Cannot execute: %%", cmd);
}

auto PlayEngine::Data::loadfile(const Mrl &mrl, int resume, int cache,
                                int edition) -> void
{
    QString file = mrl.isLocalFile() ? mrl.toLocalFile() : mrl.toString();
    if (file.isEmpty())
        return;
    timing = false;
    OptionList opts;
    opts.add("audio-device"_b, audioDevice.toLatin1(), true);
    if (hwaccCodecs.isEmpty() || hwaccBackend == HwAcc::None)
        opts.add("hwdec"_b, "no"_b);
    else {
        const auto name = HwAcc::backendName(hwaccBackend);
        opts.add("hwdec"_b, name.toLatin1());
        opts.add("hwdec-codecs"_b, hwaccCodecs, true);
    }

    if (mrl.isDisc()) {
        file = mrl.titleMrl(edition >= 0 ? edition : -1).toString();
        initSeek = resume;
    } else {
        if (edition >= 0)
            opts.add("edition"_b, edition);
        if (resume > 0)
            opts.add("start"_b, resume/1000.0);
        initSeek = -1;
    }
    opts.add("deinterlace"_b, deint != DeintMode::None);
    opts.add("volume"_b, mpVolume());
    opts.add("mute"_b, muted);
    opts.add("audio-delay"_b, audioSync/1000.0);
    opts.add("sub-delay"_b, subDelay/1000.0);

    const auto &font = subStyle.font;
    opts.add("sub-text-color"_b, font.color.name(QColor::HexArgb).toLatin1());
    QStringList fontStyles;
    if (font.bold())
        fontStyles.append(u"Bold"_q);
    if (font.italic())
        fontStyles.append(u"Italic"_q);
    QString family = font.family();
    if (!fontStyles.isEmpty())
        family += ":style="_a % fontStyles.join(' '_q);
    double factor = font.size;
    if (font.scale == OsdScalePolicy::Width)
        factor *= 1280;
    else if (font.scale == OsdScalePolicy::Diagonal)
        factor *= sqrt(1280*1280 + 720*720);
    else
        factor *= 720.0;
    opts.add("sub-text-font"_b, family.toLatin1(), true);
    opts.add("sub-text-font-size"_b, factor);
    const auto &outline = subStyle.outline;
    const auto scaled = [factor] (double v)
        { return qBound(0., v*factor, 10.); };
    const auto color = [] (const QColor &color)
        { return color.name(QColor::HexArgb).toLatin1(); };
    if (outline.enabled) {
        opts.add("sub-text-border-size"_b, scaled(outline.width));
        opts.add("sub-text-border-color"_b, color(outline.color));
    } else
        opts.add("sub-text-border-size"_b, 0.0);
    const auto &bbox = subStyle.bbox;
    if (bbox.enabled)
        opts.add("sub-text-back-color"_b, color(bbox.color));
    auto norm = [] (const QPointF &p)
        { return sqrt(p.x()*p.x() + p.y()*p.y()); };
    const auto &shadow = subStyle.shadow;
    if (shadow.enabled) {
        opts.add("sub-text-shadow-color"_b, color(shadow.color));
        opts.add("sub-text-shadow-offset"_b, scaled(norm(shadow.offset)));
    } else
        opts.add("sub-text-shadow-offset"_b, 0.0);

    if (cache > 0) {
        opts.add("cache"_b, cache);
        QByteArray value = "no"_b;
        if (cacheForPlayback > 0)
            value.setNum(qMax<int>(1, cacheForPlayback*0.01));
        opts.add("cache-pause"_b, value);
        opts.add("cache-min"_b, cacheForPlayback);
        opts.add("cache-seek-min"_b, cacheForSeeking);
    } else
        opts.add("cache"_b, "no"_b);
    opts.add("pause"_b, p->isPaused() || hasImage);
    opts.add("audio-channels"_b, ChannelLayoutInfo::data(layout), true);
    opts.add("af"_b, af(), true);
    opts.add("vf"_b, vf(), true);
    _Debug("Load: %% (%%)", file, opts.get());
    tellmpv("loadfile"_b, file.toLocal8Bit(), "replace"_b, opts.get());
}

auto PlayEngine::Data::updateMrl() -> void
{
    hasImage = startInfo.mrl.isImage();
    updateMediaName();
    emit p->mrlChanged(startInfo.mrl);
}

auto PlayEngine::Data::loadfile(int resume) -> void
{
    if (startInfo.isValid())
        loadfile(startInfo.mrl, resume, startInfo.cache, startInfo.edition);
}

auto PlayEngine::Data::updateMediaName(const QString &name) -> void
{
    mediaName = name;
    QString category;
    auto mrl = p->mrl();
    if (mrl.isLocalFile())
        category = tr("File");
    else if (mrl.isDvd())
        category = u"DVD"_q;
    else if (mrl.isBluray())
        category = tr("Blu-ray");
    else
        category = u"URL"_q;
    const QString display = name.isEmpty() ? mrl.displayName() : name;
    mediaInfo.setName(category % ": "_a % display);
}
