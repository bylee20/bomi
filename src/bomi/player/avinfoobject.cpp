#include "avinfoobject.hpp"
#include "streamtrack.hpp"
#include "video/videoformat.hpp"
#include "audio/audioformat.hpp"

SIA updateTracks(QVector<AvTrackInfoObject*> &objs, const StreamList &tracks) -> StreamTrack
{
    qDeleteAll(objs); objs.clear(); objs.reserve(tracks.size());
    for (auto &track : tracks)
        objs.push_back(new AvTrackInfoObject(track));
    auto it = _FindSelectedTrack(tracks);
    return it != tracks.end() ? *it : StreamTrack();
}

auto CodecInfoObject::parse(const QString &info) -> void
{
    QRegEx regex(uR"(^([^\[\]]+) \[([^:]+):([^\]]+)\]$)"_q);
    const auto match = regex.match(info);
    if (match.hasMatch()) {
        if (_Change(m_desc, match.captured(1)))
            emit descriptionChanged();
        setFamily(match.captured(2));
        setType(match.captured(3));
    }
}

auto AvTrackInfoObject::set(const StreamTrack &track) -> void
{
    setId(track.id());
    setCodec(track.codec());
    setTitle(track.title());
    setLanguage(track.language());
    setSelected(track.isSelected());
}

auto AvTrackInfoObject::set(const AvTrackInfoObject *track) -> void
{
    if (track) {
        setId(track->m_id);
        setCodec(track->m_codec);
        setTitle(track->m_title);
        setLanguage(track->m_lang);
        setSelected(track->m_selected);
    } else
        set(StreamTrack());
}

template<class L, class T = typename std::remove_pointer<typename L::value_type>::type>
auto makeQmlList(const QObject *o, const L *list) -> QQmlListProperty<T>
{
    auto at = [] (QQmlListProperty<T> *p, int index) -> T*
        { return static_cast<const L*>(p->data)->value(index); };
    auto count = [] (QQmlListProperty<T> *p) -> int
        { return static_cast<const L*>(p->data)->size(); };
    return QQmlListProperty<T>(const_cast<QObject*>(o), const_cast<L*>(list), count, at);
}

auto AvCommonInfoObject::tracks() const -> QQmlListProperty<AvTrackInfoObject>
{
    return makeQmlList(this, &m_tracks);
}

auto AvCommonInfoObject::setTracks(const StreamList &tracks) -> void
{
    m_track.set(updateTracks(m_tracks, tracks));
    emit tracksChanged();
}

auto AvCommonInfoObject::setTrack(const StreamTrack &track) -> void
{
    m_track.set(track);
}

/******************************************************************************/

auto AudioFormatInfoObject::setFormat(const AudioFormat &format) -> void
{
    setBitrate(format.bitrate());
    setSampleRate(format.samplerate(), false);
    setChannels(format.channels(), format.nch());
    setType(format.type());
    setDepth(format.bits());
}

auto AudioInfoObject::setDriver(const QString &driver) -> void
{
    if (_Change(m_driver, driver)) {
        emit driverChanged();
        emit deviceChanged();
    }
}

auto AudioInfoObject::setDevice(const QString &device) -> void
{
    if (_Change(m_device, device))
        emit deviceChanged();
}

auto AudioInfoObject::device() const -> QString
{
    if (m_device.startsWith(m_driver % u'/'_q))
        return m_device.mid(m_driver.size() + 1);
    return u"auto"_q;
}

/******************************************************************************/

auto VideoFormatInfoObject::rangeText() const -> QString
{
    switch (m_range) {
    case ColorRange::Limited:
        return u"Limited"_q;
    case ColorRange::Full:
        return u"Full"_q;
    case ColorRange::Auto:
        return u"--"_q;
    }
    return QString();
}

auto VideoFormatInfoObject::spaceText() const -> QString
{
    switch (m_space) {
    case ColorSpace::BT601:
        return u"BT.601"_q;
    case ColorSpace::BT709:
        return u"BT.709"_q;
    case ColorSpace::SMPTE240M:
        return u"SMPTE-240M"_q;
    case ColorSpace::YCgCo:
        return u"YCgCo"_q;
    case ColorSpace::RGB:
        return u"RGB"_q;
    case ColorSpace::XYZ:
        return u"XYZ"_q;
    case ColorSpace::BT2020NCL:
        return u"BT.2020-NCL"_q;
    case ColorSpace::BT2020CL:
        return u"BT.2020-CL"_q;
    case ColorSpace::Auto:
        return u"--"_q;
    }
    return QString();
}

VideoInfoObject::VideoInfoObject()
{
    connect(&m_output, &VideoFormatInfoObject::fpsChanged,
            this, &VideoInfoObject::delayedTimeChanged);
    connect(this, &VideoInfoObject::delayedFramesChanged,
            this, &VideoInfoObject::delayedTimeChanged);
    connect(&m_timer, &QTimer::timeout, this, [=] () {
        auto fps = 0.0;
        if (m_dropped)
            fps = m_dropped / (m_time.elapsed() * 1e-3);
        if (_Change(m_droppedFps, fps))
            emit droppedFpsChanged();
    });
    m_timer.setInterval(100);
}

void VideoInfoObject::setDroppedFrames(int f)
{
    if (f > 0 && m_dropped < 1) {
        QMetaObject::invokeMethod(&m_timer, "start");
        m_time.restart();
    } else if (f < 1 && m_timer.isActive()) {
        QMetaObject::invokeMethod(&m_timer, "stop");
        if (_Change(m_droppedFps, 0.0))
            emit droppedFpsChanged();
    }
    if (_Change(m_dropped, f))
        emit droppedFramesChanged();
}

/******************************************************************************/

SubtitleInfoObject::SubtitleInfoObject()
{
    auto find = [] (const QVector<AvTrackInfoObject*> &list) -> int {
        for (int i = list.size() - 1; i >= 0; --i)
            if (list[i]->isSelected())
                return i;
        return -1;
    };
    auto updateTrack = [=] () {
        int idx = find(getTracks());
        if (idx != -1) {
            track()->set(getTracks()[idx]);
            idx += m_files.size();
        } else {
            idx = find(m_files);
            if (idx != -1)
                track()->set(m_files[idx]);
        }
        if (_Change(m_total, getTracks().size() + m_files.size()))
            emit totalLengthChanged();
        if (_Change(m_id, idx + 1))
            emit currentNumberChanged();
    };
    connect(this, &SubtitleInfoObject::tracksChanged, this, updateTrack);
    connect(this, &SubtitleInfoObject::filesChanged, this, updateTrack);
}

auto SubtitleInfoObject::files() const -> QQmlListProperty<AvTrackInfoObject>
{
    return makeQmlList(this, &m_files);
}

auto SubtitleInfoObject::setFiles(const StreamList &files) -> void
{
    updateTracks(m_files, files);
    emit filesChanged();
}
