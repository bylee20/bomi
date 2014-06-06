#ifndef MRLSTATE_HPP
#define MRLSTATE_HPP

#include "mrl.hpp"
#include "enum/clippingmethod.hpp"
#include "enum/interpolatortype.hpp"
#include "enum/videoratio.hpp"
#include "enum/verticalalignment.hpp"
#include "enum/horizontalalignment.hpp"
#include "enum/colorrange.hpp"
#include "enum/colorspace.hpp"
#include "enum/deintmode.hpp"
#include "enum/dithering.hpp"
#include "enum/channellayout.hpp"
#include "enum/subtitledisplay.hpp"
#include "enum/videoeffect.hpp"
#include "video/videocolor.hpp"
#include "subtitle/submisc.hpp"

static inline bool operator == (const QMetaProperty &lhs, const QMetaProperty &rhs) {
    return lhs.enclosingMetaObject() == rhs.enclosingMetaObject() && lhs.propertyIndex() == rhs.propertyIndex();
}

class MrlStateV2 : public QObject {
    Q_OBJECT
    Q_PROPERTY(Mrl mrl MEMBER mrl)
    Q_PROPERTY(QString device MEMBER device)
    Q_PROPERTY(QDateTime last_played_date_time MEMBER last_played_date_time)
    Q_PROPERTY(int resume_position MEMBER resume_position)
    Q_PROPERTY(int edition MEMBER edition)
    Q_PROPERTY(int play_speed MEMBER play_speed NOTIFY playSpeedChanged)

    Q_PROPERTY(InterpolatorType video_interpolator MEMBER video_interpolator NOTIFY videoInterpolatorChanged)
    Q_PROPERTY(InterpolatorType video_chroma_upscaler MEMBER video_chroma_upscaler NOTIFY videoChromaUpscalerChanged)
    Q_PROPERTY(VideoRatio video_aspect_ratio MEMBER video_aspect_ratio NOTIFY videoAspectRatioChanged)
    Q_PROPERTY(VideoRatio video_crop_ratio MEMBER video_crop_ratio NOTIFY videoCropRatioChanged)
    Q_PROPERTY(DeintMode video_deinterlacing MEMBER video_deinterlacing NOTIFY videoDeinterlacingChanged)
    Q_PROPERTY(Dithering video_dithering MEMBER video_dithering NOTIFY videoDitheringChanged)
    Q_PROPERTY(QPoint video_offset MEMBER video_offset NOTIFY videoOffsetChanged)
    Q_PROPERTY(VerticalAlignment video_vertical_alignment MEMBER video_vertical_alignment NOTIFY videoVerticalAlignmentChanged)
    Q_PROPERTY(HorizontalAlignment video_horizontal_alignment MEMBER video_horizontal_alignment NOTIFY videoHorizontalAlignmentChanged)
    Q_PROPERTY(VideoColor video_color MEMBER video_color NOTIFY videoColorChanged)
    Q_PROPERTY(ColorRange video_range MEMBER video_range NOTIFY videoRangeChanged)
    Q_PROPERTY(ColorSpace video_space MEMBER video_space NOTIFY videoSpaceChanged)

    Q_PROPERTY(int audio_volume MEMBER audio_volume NOTIFY audioVolumeChanged)
    Q_PROPERTY(int audio_amplifier MEMBER audio_amplifier NOTIFY audioAmpChanged)
    Q_PROPERTY(int audio_sync MEMBER audio_sync NOTIFY audioSyncChanged REVISION 1)
    Q_PROPERTY(int audio_track MEMBER audio_track REVISION 1)
    Q_PROPERTY(bool audio_muted MEMBER audio_muted NOTIFY audioMutedChanged)
    Q_PROPERTY(bool audio_volume_normalizer MEMBER audio_volume_normalizer NOTIFY audioVolumeNormalizerChanged)
    Q_PROPERTY(bool audio_tempo_scaler MEMBER audio_tempo_scaler NOTIFY audioTempoScalerChanged)
    Q_PROPERTY(ChannelLayout audio_channel_layout MEMBER audio_channel_layout NOTIFY audioChannelLayoutChanged)

    Q_PROPERTY(VerticalAlignment sub_alignment MEMBER sub_alignment NOTIFY subAlignmentChanged)
    Q_PROPERTY(SubtitleDisplay sub_display MEMBER sub_display NOTIFY subDisplayChanged)
    Q_PROPERTY(int sub_position MEMBER sub_position NOTIFY subPositionChanged)
    Q_PROPERTY(int sub_sync MEMBER sub_sync NOTIFY subSyncChanged REVISION 1)
    Q_PROPERTY(SubtitleStateInfo sub_track MEMBER sub_track REVISION 1)
public:
    Mrl mrl;
    QString device;

// play state
    QDateTime last_played_date_time;
    int resume_position = 0;
    int play_speed = 100;
    int edition = -1;

// video state
    VideoRatio video_aspect_ratio = VideoRatio::Source;
    VideoRatio video_crop_ratio = VideoRatio::Source;
    DeintMode video_deinterlacing = DeintMode::Auto;
    InterpolatorType video_interpolator = InterpolatorType::Bilinear;
    InterpolatorType video_chroma_upscaler = InterpolatorType::Bilinear;
    Dithering video_dithering = Dithering::Fruit;
    QPoint video_offset = {0, 0};
    VerticalAlignment video_vertical_alignment = VerticalAlignment::Center;
    HorizontalAlignment video_horizontal_alignment = HorizontalAlignment::Center;
    VideoColor video_color = {0, 0, 0, 0};
    ColorRange video_range = ColorRange::Auto;
    ColorSpace video_space = ColorSpace::Auto;
    VideoEffects video_effects = 0;

// audio state
    int audio_amplifier = 100;
    int audio_volume = 100, audio_sync = 0;
    bool audio_muted = false, audio_volume_normalizer = false, audio_tempo_scaler = true;
    ChannelLayout audio_channel_layout = ChannelLayoutInfo::default_();
    int audio_track = -1;

// subtitle state
    int sub_position = 100;
    int sub_sync = 0;
    SubtitleDisplay sub_display = SubtitleDisplay::OnLetterbox;
    VerticalAlignment sub_alignment = VerticalAlignment::Bottom;
    SubtitleStateInfo sub_track;

    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;

    static const int Version = 2;
    struct PropertyInfo {
        QMetaProperty property;
        QString description;
    };
    static auto restorableProperties() -> QVector<PropertyInfo>;
signals:
    void playSpeedChanged();

    void videoInterpolatorChanged();
    void videoAspectRatioChanged();
    void videoCropRatioChanged();
    void videoDeinterlacingChanged();
    void videoChromaUpscalerChanged();
    void videoDitheringChanged();
    void videoColorChanged(const VideoColor &color);
    void videoOffsetChanged();
    void videoVerticalAlignmentChanged();
    void videoHorizontalAlignmentChanged();
    void videoRangeChanged();
    void videoSpaceChanged();

    void audioVolumeChanged();
    void audioAmpChanged();
    void audioMutedChanged();
    void audioSyncChanged();
    void audioVolumeNormalizerChanged();
    void audioTempoScalerChanged();
    void audioChannelLayoutChanged();
    void audioTrackChanged();

    void subPositionChanged();
    void subSyncChanged();
    void subDisplayChanged();
    void subAlignmentChanged();
};

using MrlState = MrlStateV2;

auto _ImportMrlStates(int version, QSqlDatabase db) -> QVector<MrlState*>;

#endif // MRLSTATE_HPP
