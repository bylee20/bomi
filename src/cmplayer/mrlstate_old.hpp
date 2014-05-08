#ifndef MRLSTATE_OLD_H
#define MRLSTATE_OLD_H

#include "stdafx.hpp"
#include "mrlstate.hpp"

class MrlStateProperty;

// REVISION means default
class MrlStateV1 : public QObject {
    Q_OBJECT
    Q_PROPERTY(Mrl mrl MEMBER mrl)
    Q_PROPERTY(QDateTime last_played_date_time MEMBER last_played_date_time)
    Q_PROPERTY(int resume_position MEMBER resume_position)
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

// play state
    QDateTime last_played_date_time;
    int resume_position = 0;
    int play_speed = 100;

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
    int video_effects = 0;

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

    static const int Version = 1;
    auto fillCurrentVersion(MrlState *state) const -> void {
        state->mrl = mrl;
        state->last_played_date_time = last_played_date_time;
        state->resume_position = resume_position;
        state->play_speed = play_speed;
        state->video_aspect_ratio = video_aspect_ratio;
        state->video_crop_ratio = video_crop_ratio;
        state->video_deinterlacing = video_deinterlacing;
        state->video_interpolator = video_interpolator;
        state->video_chroma_upscaler = video_chroma_upscaler;
        state->video_dithering = video_dithering;
        state->video_offset = video_offset;
        state->video_vertical_alignment = video_vertical_alignment;
        state->video_horizontal_alignment = video_horizontal_alignment;
        state->video_color = video_color;
        state->video_range = video_range;
        state->video_effects = video_effects;
        state->audio_amplifier = audio_amplifier;
        state->audio_volume = audio_volume;
        state->audio_sync = audio_sync;
        state->audio_muted = audio_muted;
        state->audio_volume_normalizer = audio_volume_normalizer;
        state->audio_tempo_scaler = audio_tempo_scaler;
        state->audio_channel_layout = audio_channel_layout;
        state->audio_track = audio_track;
        state->sub_position = sub_position;
        state->sub_sync = sub_sync;
        state->sub_display = sub_display;
        state->sub_alignment = sub_alignment;
        state->sub_track = sub_track;
    }
signals:
    void playSpeedChanged();

    void videoInterpolatorChanged();
    void videoAspectRatioChanged();
    void videoCropRatioChanged();
    void videoDeinterlacingChanged();
    void videoChromaUpscalerChanged();
    void videoDitheringChanged();
    void videoColorChanged();
    void videoOffsetChanged();
    void videoVerticalAlignmentChanged();
    void videoHorizontalAlignmentChanged();
    void videoRangeChanged();

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

struct MrlFieldV1 {
    auto type() const -> QString { return m_type; }
    auto toSql(const QVariant &var) const -> QString { return m_toSql(var); }
    auto fromSql(const QVariant &var) const -> QVariant;
    auto property() const -> const QMetaProperty& { return m_property; }
    auto default_() const -> QVariant { return QVariant(); }
    static auto list() -> QList<MrlFieldV1>;
private:
    static auto pass(const QVariant &var, int) -> QVariant { return var; }
    QString m_type;
    QMetaProperty m_property;
    QString (*m_toSql)(const QVariant&);
    QVariant (*m_fromSql)(const QVariant&, int) = pass;
};

inline auto MrlFieldV1::fromSql(const QVariant &var) const -> QVariant
{ return m_fromSql(var, m_property.userType()); }

#endif // MRLSTATE_OLD_H
