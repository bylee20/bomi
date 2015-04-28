#ifndef MRLSTATE_HPP
#define MRLSTATE_HPP

#include "mrl.hpp"
#include "tmp/type_traits.hpp"
#include "player/streamtrack.hpp"
#include "video/videocolor.hpp"
#include "audio/audioequalizer.hpp"
#include "enum/interpolator.hpp"
#include "enum/videoratio.hpp"
#include "enum/verticalalignment.hpp"
#include "enum/horizontalalignment.hpp"
#include "enum/colorrange.hpp"
#include "enum/colorspace.hpp"
#include "enum/deintmode.hpp"
#include "enum/videoeffect.hpp"
#include "enum/dithering.hpp"
#include "enum/channellayout.hpp"
#include "enum/subtitledisplay.hpp"
#include "enum/rotation.hpp"
#include <QMetaProperty>

struct CacheInfo {
    struct Item { double sec = 10; qint64 kb = 0; bool file = false; };
    auto get(const Mrl &mrl) const -> const Item&
    {
        Mrl file = mrl;
        if (mrl.isCueTrack())
            file = Mrl(mrl.toCueTrack().file);
        if (file.isLocalFile()) {
            auto path = file.toLocalFile();
            for (auto &folder : remotes) {
                if (path.startsWith(folder))
                    return network;
            }
            return local;
        }
        if (mrl.isDisc())
            return disc;
        return network;
    }
    auto playback_kb(qint64 cache) const -> qint64
        { return qBound<qint64>(0, min_playback_kb, cache * 0.5); }
    auto seeking_kb(qint64 cache) const -> qint64
        { return qBound<qint64>(0, min_seeking_kb, cache * 0.5); }
    Item local, network, disc;
    qint64 file_kb = 1024 * 1024, min_playback_kb = 0, min_seeking_kb = 500;
    QStringList remotes;
};

struct SambaInfo {
    QString username, password, workgroup;
};

SIA qBound(const QPointF &min, const QPointF &val, const QPointF &max) -> QPointF
{
    return QPointF(qBound(min.x(), val.x(), max.x()), qBound(min.y(), val.y(), max.y()));
}

class MrlStateV3;

class MrlState : public QObject {
    Q_OBJECT
#define P_GEN(type, name, def, checked_t, desc, rev) \
private: \
    type m_##name = def; \
    Q_PROPERTY(type name READ name WRITE set_##name NOTIFY name ## _changed REVISION rev) \
    Q_CLASSINFO(#name, desc) \
public: \
    Q_SIGNAL void name ## _changed(type); \
    Q_INVOKABLE QString desc_ ## name() const { return tr(desc); } \
    type name() const { return m_##name; } \
    bool set_##name(tmp::cval_t<type> t) \
    { \
        bool ret = false; \
        if (m_mutex) m_mutex->lock(); \
        ret = _Change(m_##name, checked_t); \
        if (m_mutex) m_mutex->unlock(); \
        emit name##_changed(m_##name); \
        return ret; \
    } \
private:
#define P_(type, name, def, desc, rev) P_GEN(type, name, def, t, desc, rev)
#define PB(type, name, def, min, max, desc, rev) P_GEN(type, name, def, qBound(min, t, max), desc, rev)

    P_(Mrl, mrl, {}, "", 0)
    P_(QString, name, {}, "", 0)
    P_(QString, device, {}, "", 0)
    P_(QDateTime, last_played_date_time, {}, "", 0)
    P_(int, resume_position, 0, "", 0)
    P_(bool, star, false, "", 0)

    P_(int, edition, -1, "", 0)
    PB(double, play_speed, 1.0, 0.01, 10.0, QT_TR_NOOP("Playback Speed"), 0)

    P_(Interpolator, video_interpolator, Interpolator::Bilinear, QT_TR_NOOP("Video Interpolator"), 0)
    P_(Interpolator, video_interpolator_down, Interpolator::Bilinear, QT_TR_NOOP("Video Downscaling Interpolator"), 0)
    P_(Interpolator, video_chroma_upscaler, Interpolator::Bilinear, QT_TR_NOOP("Video Chroma Upscaler"), 0)
    P_(double, video_aspect_ratio, _EnumData(VideoRatio::Source), QT_TR_NOOP("Video Aspect Ratio"), 0)
    P_(double, video_crop_ratio, _EnumData(VideoRatio::Source), QT_TR_NOOP("Video Crop Ratio"), 0)
    P_(Rotation, video_rotation, Rotation::D0, QT_TR_NOOP("Video Rotation"), 1);
    P_(DeintMode, video_deinterlacing, DeintMode::Auto, QT_TR_NOOP("Video Deinterlacing"), 0)
    P_(Dithering, video_dithering, Dithering::None, QT_TR_NOOP("Video Dithering"), 0)
    PB(double, video_zoom, 1, 0.1, 10.0, QT_TR_NOOP("Video Screen Zoom"), 0);
    PB(QPointF, video_offset, {}, QPointF(-1, -1), QPointF(1, 1), QT_TR_NOOP("Video Screen Position"), 0)
    P_(VerticalAlignment, video_vertical_alignment, VerticalAlignment::Center, QT_TR_NOOP("Video Vertical Alignment"), 0)
    P_(HorizontalAlignment, video_horizontal_alignment, HorizontalAlignment::Center, QT_TR_NOOP("Video Horizontal Alignment"), 0)
    P_(VideoColor, video_color, {}, QT_TR_NOOP("Video Color Adjustment"), 0)
    P_(ColorRange, video_range, ColorRange::Auto, QT_TR_NOOP("Video Color Range"), 0)
    P_(ColorSpace, video_space, ColorSpace::Auto, QT_TR_NOOP("Video Color Space"), 0)
    P_(bool, video_hq_upscaling, false, QT_TR_NOOP("Video High Quality Upscaling"), 0)
    P_(bool, video_hq_downscaling, false, QT_TR_NOOP("Video High Quality Downscaling"), 0)
    P_(bool, video_motion_interpolation, false, QT_TR_NOOP("Video Motion Smoothing"), 0)
    P_(VideoEffects, video_effects, 0, QT_TR_NOOP("Video Effects"), 0)
    P_(StreamList, video_tracks, {StreamVideo}, QT_TR_NOOP("Video Track"), 0)

    PB(double, audio_volume, 1.0, 0.0, 1.0, QT_TR_NOOP("Audio Volume"), 0)
    PB(double, audio_amplifier, 1.0, 0.0, 10.0, QT_TR_NOOP("Audio Amp"), 0)
    P_(AudioEqualizer, audio_equalizer, {}, QT_TR_NOOP("Audio Equalizer"), 0)
    P_(int, audio_sync, 0, QT_TR_NOOP("Audio Sync"), 1)
    P_(StreamList, audio_tracks, {StreamAudio},  QT_TR_NOOP("Audio Tracks"), 1)
    P_(bool, audio_muted, false, QT_TR_NOOP("Audio Mute"), 0)
    P_(bool, audio_volume_normalizer, false, QT_TR_NOOP("Audio Volume Normalizer"), 0)
    P_(bool, audio_tempo_scaler, true, QT_TR_NOOP("Audio Tempo Scaler"), 0)
    P_(ChannelLayout, audio_channel_layout, ChannelLayoutInfo::default_(), QT_TR_NOOP("Audio Channel Layout"), 0)

    P_(VerticalAlignment, sub_alignment, VerticalAlignment::Bottom, QT_TR_NOOP("Subtitle Alignment"), 0)
    P_(SubtitleDisplay, sub_display, SubtitleDisplay::OnLetterbox, QT_TR_NOOP("Subtitle Display"), 0)
    PB(double, sub_position, 1.0, 0.0, 1.0, QT_TR_NOOP("Subtitle Position"), 0)
    P_(int, sub_sync, 0, QT_TR_NOOP("Subtitle Sync"), 0)
    P_(StreamList, sub_tracks, {StreamSubtitle}, QT_TR_NOOP("Subtitle Tracks"), 1)
    P_(StreamList, sub_tracks_inclusive, {StreamInclusiveSubtitle}, "", 1)
    P_(bool, sub_hidden, false, QT_TR_NOOP("Subtitle Hiding"), 0)
    P_(bool, sub_style_overriden, false, QT_TR_NOOP("Override ASS Text Style"), 0)
    P_(bool, sub_override_ass_position, false, QT_TR_NOOP("Override ASS Position"), 0);
    P_(bool, sub_override_ass_scale, false, QT_TR_NOOP("Override ASS Scale"), 0);
    PB(double, sub_scale, 0.0, -1.0, 1.0, QT_TR_NOOP("Subtitle Scale"), 0)
public:
    static const int Version = 4;
    MrlState();
    ~MrlState();
    struct PropertyInfo { QString property; QString description; };
    static auto description(const char *property) -> QString;
    auto description(StreamType type) const -> QString { return (this->*(m_tracks[type].desc))(); }
    auto signal(StreamType type) const -> void(MrlState::*)(StreamList) { return m_tracks[type].signal; }
    auto notifySignal(const char *property) const -> QMetaMethod;
    auto metaProperty(const char *property) const -> QMetaProperty;
    auto tracks(StreamType type) const -> const StreamList& { return *m_tracks[type].tracks; }
    auto select(StreamType type, int id) -> void;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    auto copyFrom(const MrlState *state) -> void;
    static auto defaultProperties() -> QStringList;
    static auto restorableProperties() -> QVector<PropertyInfo>;
    static auto table() -> QString { return "state"_a % _N(Version); }
    auto import(const MrlStateV3 *v3) -> void;
signals:
    void tracksChanged(StreamType type);
    void currentTrackChanged(StreamType type);
private:
    auto notifyAll() const -> void;
    template<class T>
    auto __set_dummy(const T &) { }
    auto __dummy_int() const -> int { return 0; }
    auto __dummy_string() const -> QString { return QString(); }
    friend class PlayEngine; friend class VideoSettings;
    struct TrackInfo {
        StreamList *tracks;
        void(MrlState::*signal)(StreamList);
        QString(MrlState::*desc)() const;
    };

    struct Data;
    Data *d;

    QMutex *m_mutex = nullptr;

    QVector<TrackInfo> m_tracks;
};
#undef P_
#undef P_GEN
#undef PB

class QSqlDatabase;

auto _ImportMrlStates(int version, QSqlDatabase db) -> QVector<MrlState*>;

#endif // MRLSTATE_HPP
