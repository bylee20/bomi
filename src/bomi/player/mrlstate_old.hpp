#ifndef MRLSTATE_OLD_HPP
#define MRLSTATE_OLD_HPP

#include "mrl.hpp"
#include "tmp/type_traits.hpp"
#include "player/streamtrack.hpp"
#include "video/videocolor.hpp"
#include "video/interpolatorparams.hpp"
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
#include <QMetaProperty>

class MrlStateV3 : public QObject {
    Q_OBJECT
    Q_PROPERTY(int audio_track READ __dummy_int WRITE __set_dummy)
    Q_PROPERTY(bool sub_visible READ __dummy_int WRITE __set_dummy)
    Q_PROPERTY(QString sub_track READ __dummy_string WRITE __set_dummy)
#define P_GEN(type, name, def, checked_t, desc, rev) \
private: \
    type m_##name = def; \
    Q_PROPERTY(type name READ name WRITE set_##name NOTIFY name ## _changed REVISION rev) \
public: \
    Q_SIGNAL void name ## _changed(type); \
    Q_INVOKABLE QString desc_ ## name() const { return tr(desc); } \
    type name() const { return m_##name; } \
    bool set_##name(tmp::cval_t<type> t) \
    { \
        bool ret = false; \
        ret = _Change(m_##name, checked_t); \
        emit name##_changed(m_##name); \
        return ret; \
    } \
private:
#define P_(type, name, def, desc, rev) P_GEN(type, name, def, t, desc, rev)
#define PB(type, name, def, min, max, desc, rev) P_GEN(type, name, def, qBound(min, t, max), desc, rev)

    P_(Mrl, mrl, {}, "", 0)
    P_(QString, device, {}, "", 0)
    P_(QDateTime, last_played_date_time, {}, "", 0)
    P_(int, resume_position, 0, "", 0)
    P_(int, edition, -1, "", 0)

    PB(int, play_speed, 100, 1, 1000, ("Playback Speed"), 0)

    P_(Interpolator, video_interpolator, Interpolator::Bilinear, ("Video Interpolator"), 0)
    P_(Interpolator, video_chroma_upscaler, Interpolator::Bilinear, ("Video Chroma Upscaler"), 0)
    P_(VideoRatio, video_aspect_ratio, VideoRatio::Source, ("Video Aspect Ratio"), 0)
    P_(VideoRatio, video_crop_ratio, VideoRatio::Source, ("Video Crop Ratio"), 0)
    P_(DeintMode, video_deinterlacing, DeintMode::Auto, ("Video Deinterlacing"), 0)
    P_(Dithering, video_dithering, Dithering::Fruit, ("Video Dithering"), 0)
    P_(QPoint, video_offset, {}, ("Video Screen Position"), 0)
    P_(VerticalAlignment, video_vertical_alignment, VerticalAlignment::Center, ("Video Vertical Alignment"), 0)
    P_(HorizontalAlignment, video_horizontal_alignment, HorizontalAlignment::Center, ("Video Horizontal Alignment"), 0)
    P_(VideoColor, video_color, {}, ("Video Color Adjustment"), 0)
    P_(ColorRange, video_range, ColorRange::Auto, ("Video Color Range"), 0)
    P_(ColorSpace, video_space, ColorSpace::Auto, ("Video Color Space"), 0)
    P_(bool, video_hq_upscaling, false, ("Video High Quality Upscaling"), 0)
    P_(bool, video_hq_downscaling, false, ("Video High Quality Downscaling"), 0)
    P_(bool, video_motion_interpolation, false, ("Video Motion Interpolation"), 0)
    P_(VideoEffects, video_effects, 0, ("Video Effects"), 0)
    P_(StreamList, video_tracks, {StreamVideo}, "", 0)

    PB(int, audio_volume, 100, 0, 100, ("Audio Volume"), 0)
    PB(int, audio_amplifier, 100, 0, 1000, ("Audio Amp"), 0)
    P_(AudioEqualizer, audio_equalizer, {}, ("Audio Equalizer"), 0)
    P_(int, audio_sync, 0, ("Audio Sync"), 1)
    P_(StreamList, audio_tracks, {StreamAudio},  ("Audio Track"), 1)
    P_(bool, audio_muted, false, ("Audio Mute"), 0)
    P_(bool, audio_volume_normalizer, false, ("Audio Volume Normalizer"), 0)
    P_(bool, audio_tempo_scaler, true, ("Audio Tempo Scaler"), 0)
    P_(ChannelLayout, audio_channel_layout, ChannelLayoutInfo::default_(), ("Audio Channel Layout"), 0)

    P_(VerticalAlignment, sub_alignment, VerticalAlignment::Bottom, ("Subtitle Alignment"), 0)
    P_(SubtitleDisplay, sub_display, SubtitleDisplay::OnLetterbox, ("Subtitle Display"), 0)
    PB(int, sub_position, 100, 0, 100, ("Subtitle Position"), 0)
    P_(int, sub_sync, 0, ("Subtitle Sync"), 0)
    P_(StreamList, sub_tracks, {StreamSubtitle}, ("Subtitle Track"), 1)
    P_(StreamList, sub_tracks_inclusive, {StreamInclusiveSubtitle}, "", 1)
    P_(bool, sub_hidden, false, ("Subtitle Hiding"), 0)
    P_(bool, sub_style_overriden, false, ("Override ASS Style"), 0)

    P_(IntrplParamSetMap, video_interpolator_map, {}, "", 0)
    P_(IntrplParamSetMap, video_chroma_upscaler_map, {}, "", 0)
public:
    static const int Version = 3;
//    auto metaProperty(const char *property) const -> QMetaProperty;
    auto setFromJson(const QJsonObject &json) -> bool;
    static auto table() -> QString { return "state"_a % _N(Version); }
private:
    auto notifyAll() const -> void;
    template<class T>
    auto __set_dummy(const T &) { }
    auto __dummy_int() const -> int { return 0; }
    auto __dummy_string() const -> QString { return QString(); }
};
#undef P_
#undef P_GEN
#undef PB

#endif // MRLSTATE_OLD_HPP
