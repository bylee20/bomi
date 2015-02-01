#include "avinfoobject.hpp"
#include "streamtrack.hpp"
#include "video/videoformat.hpp"
#include "audio/audioformat.hpp"

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

auto AvTrackInfoObject::fromTrack(int n, const StreamTrack &track) -> AvTrackInfoObject*
{
    AvTrackInfoObject *info = new AvTrackInfoObject;
    info->m_id = track.id();
    info->m_number = n;
    info->m_codec = track.codec();
    info->m_title = track.title();
    info->m_lang = track.language();
    info->m_selected = track.isSelected();
    return info;
}

auto AvCommonInfoObject::track() const -> AvTrackInfoObject*
{
    if (m_track)
        return m_track;
    static AvTrackInfoObject dummy;
    return &dummy;
}

auto AvCommonInfoObject::update(const StreamList &tracks, bool clear) -> AvTrackInfoObject*
{
    if (clear) {
        qDeleteAll(m_tracks);
        m_tracks.clear();
    }
    m_tracks.reserve(m_tracks.size() + tracks.size());
    AvTrackInfoObject *sel = nullptr;
    for (auto track : tracks) {
        m_tracks.push_back(AvTrackInfoObject::fromTrack(m_tracks.size() + 1, track));
        if (track.isSelected())
            sel = m_tracks.back();
    }
    return sel;
}

auto AvCommonInfoObject::tracks() const -> QQmlListProperty<AvTrackInfoObject>
{
    return _MakeQmlList(this, &m_tracks);
}

auto AvCommonInfoObject::setTracks(const StreamList &tracks) -> void
{
    m_track = update(tracks);
    emit tracksChanged();
    emit trackChanged();
}

auto AvCommonInfoObject::setTracks(const StreamList &tracks1, const StreamList &tracks2) -> void
{
    auto sel = update(tracks1);
    if (auto sel2 = update(tracks2, false))
        sel = sel2;
    m_track = sel;
    emit tracksChanged();
    emit trackChanged();
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
}
