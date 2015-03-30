#include "avinfoobject.hpp"
#include "streamtrack.hpp"
#include "video/videoformat.hpp"
#include "audio/audioformat.hpp"
#include <QQmlEngine>

template<class L, class T = typename std::remove_pointer<typename L::value_type>::type>
static inline auto _MakeQmlList(const QObject *o, const L *list) -> QQmlListProperty<T>
{
    auto at = [] (QQmlListProperty<T> *p, int index) -> T*
        { return static_cast<const L*>(p->data)->value(index); };
    auto count = [] (QQmlListProperty<T> *p) -> int
        { return static_cast<const L*>(p->data)->size(); };
    return QQmlListProperty<T>(const_cast<QObject*>(o), const_cast<L*>(list), count, at);
}

auto CodecObject::parse(const QString &info) -> void
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

auto AvTrackObject::typeText() const -> QString
{
    return StreamTrack::typeDescription((StreamType)m_type, m_albumart);
}

auto AvTrackObject::fromTrack(int n, const StreamTrack &track) -> AvTrackObject*
{
    AvTrackObject *info = new AvTrackObject;
    info->m_id = track.id();
    info->m_number = n;
    info->m_codec = track.codec();
    info->m_title = track.title();
    info->m_lang = track.language();
    info->m_selected = track.isSelected();
    info->m_enc = track.encoding().name();
    info->m_albumart = track.isAlbumArt();
    info->m_type = track.type();
    return info;
}

AvCommonObject::AvCommonObject(int type)
{
    m_dummy.m_type = type;
}

AvCommonObject::~AvCommonObject()
{

}

auto AvCommonObject::track() const -> AvTrackObject*
{
    if (m_track)
        return m_track;
    return &m_dummy;
}

auto AvCommonObject::update(const StreamList &tracks, bool clear) -> AvTrackObject*
{
    if (clear) {
        for (auto track : m_tracks) {
            QQmlEngine::setObjectOwnership(track, QQmlEngine::JavaScriptOwnership);
        }
//        qDeleteAll(m_tracks);
        m_tracks.clear();
    }
    m_tracks.reserve(m_tracks.size() + tracks.size());
    AvTrackObject *sel = nullptr;
    for (auto track : tracks) {
        m_tracks.push_back(AvTrackObject::fromTrack(m_tracks.size() + 1, track));
        if (track.isSelected())
            sel = m_tracks.back();
    }
    return sel;
}

auto AvCommonObject::tracks() const -> QQmlListProperty<AvTrackObject>
{
    return _MakeQmlList(this, &m_tracks);
}

auto AvCommonObject::setTracks(const StreamList &tracks) -> void
{
    m_track = update(tracks);
    emit tracksChanged();
    emit trackChanged();
}

/******************************************************************************/

AudioObject::AudioObject()
    : AvCommonObject(StreamAudio)
{

}

auto AudioFormatObject::setFormat(const AudioFormat &format) -> void
{
    setBitrate(format.bitrate());
    setSampleRate(format.samplerate(), false);
    setChannels(format.channels(), format.nch());
    setType(format.type());
    setDepth(format.bits());
}

auto AudioObject::setDriver(const QString &driver) -> void
{
    if (_Change(m_driver, driver)) {
        emit driverChanged();
        emit deviceChanged();
    }
}

auto AudioObject::setDevice(const QString &device) -> void
{
    if (_Change(m_device, device))
        emit deviceChanged();
}

auto AudioObject::device() const -> QString
{
    if (m_device.startsWith(m_driver % u'/'_q))
        return m_device.mid(m_driver.size() + 1);
    return u"auto"_q;
}

/******************************************************************************/

auto VideoFormatObject::rangeText() const -> QString
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

auto VideoFormatObject::spaceText() const -> QString
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

VideoObject::VideoObject()
    : AvCommonObject(StreamVideo)
{
    connect(this, &VideoObject::delayedFramesChanged,
            this, &VideoObject::delayedTimeChanged);
}

void VideoObject::setDroppedFrames(int f)
{
    if (f > 0 && m_dropped < 1)
        m_time.restart();
    if (_Change(m_dropped, f))
        emit droppedFramesChanged();
    auto fps = 0.0;
    if (m_dropped)
        fps = m_dropped / (m_time.elapsed() * 1e-3);
    if (_Change(m_droppedFps, fps))
        emit droppedFpsChanged();
}

auto VideoObject::delayedTime() const -> qreal
{
    double fps = m_filter.fps();
    if (m_fpsMp < 1)
        ;
    else if (m_fpsMp < 10)
        fps *= m_fpsMp;
    else
        fps = m_fpsMp;
    return fps > 1 ? (m_delayed / fps) * 1e3 : 0.0;
}

/******************************************************************************/

SubtitleObject::SubtitleObject()
    : AvCommonObject(StreamSubtitle)
{
}

auto SubtitleObject::setTracks(const StreamList &tracks1, const StreamList &tracks2) -> void
{
    auto sel = update(tracks1);
    if (auto sel2 = update(tracks2, false))
        sel = sel2;
    setTrack(sel);
    m_selection.clear();
    for (auto track : trackObjects()) {
        if (track->isSelected())
            m_selection.push_back(track);
    }
    emit tracksChanged();
    emit trackChanged();
    emit selectionChanged();
}

auto SubtitleObject::selection() const -> QQmlListProperty<AvTrackObject>
{
    return _MakeQmlList(this, &m_selection);
}
