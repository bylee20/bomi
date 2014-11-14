#include "mediamisc.hpp"
#include "playengine.hpp"
#include "audio/audiocontroller.hpp"
#include "video/videoformat.hpp"
#include <video/img_format.h>

auto CodecInfoObject::parse(const QString &info) -> void
{
    QRegEx regex(uR"(^([^\[\]]+) \[([^:]+):([^\]]+)\]$)"_q);
    const auto match = regex.match(info);
    if (match.hasMatch()) {
        if (_Change(m_desc, match.captured(1)))
            emit descriptionChanged();
        if (_Change(m_family, match.captured(2)))
            emit familyChanged();
        if (_Change(m_type, match.captured(3)))
            emit typeChanged();
    }
}

ChapterInfoObject::ChapterInfoObject(const PlayEngine *engine, QObject *parent)
: TrackInfoObject(parent), m_engine(engine) {
    connect(engine, &PlayEngine::currentChapterChanged, this, &ChapterInfoObject::setCurrent);
}

auto ChapterInfoObject::time(int i) const -> int
{
    return m_engine->chapters().value(i).time();
}

auto ChapterInfoObject::name(int i) const -> QString
{
    return m_engine->chapters().value(i).name();
}

AudioTrackInfoObject::AudioTrackInfoObject(const PlayEngine *engine, QObject *parent)
: TrackInfoObject(parent) {
    connect(engine, &PlayEngine::currentAudioStreamChanged,
            this, &AudioTrackInfoObject::setCurrent);
}

auto VideoFormatInfoObject::setImgFmt(int imgfmt) -> void
{
    char name[32] = {0};
    mp_imgfmt_to_name_buf(name, 32, imgfmt);
    AvCommonFormatObject::setType(QString::fromLatin1(name));
    const auto desc = mp_imgfmt_get_desc(imgfmt);
    const auto bpp = VideoFormat::bpp(desc);
    if (_Change(m_bpp, bpp))
        emit bppChanged();
    setDepth(IMGFMT_IS_HWACCEL(imgfmt) ? 8 : desc.plane_bits);
    updateBitrate();
}

auto VideoFormatInfoObject::rangeText() const -> QString
{
    switch (m_range) {
    case ColorRange::Limited:
        return u"Limited"_q;
    case ColorRange::Full:
        return u"Full"_q;
    case ColorRange::Extended:
        return u"Extended"_q;
    case ColorRange::Remap:
        return u"Remapped"_q;
    default:
        return u"--"_q;
    }
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
    default:
        return u"--"_q;
    }
}

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
