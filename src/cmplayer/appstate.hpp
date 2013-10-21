#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "stdafx.hpp"
#include "enums.hpp"
#include "videocolor.hpp"

class AppState : public QObject {
	Q_OBJECT
	Q_PROPERTY(InterpolatorType video_interpolator MEMBER video_interpolator NOTIFY videoInterpolatorChanged)
	Q_PROPERTY(InterpolatorType video_chroma_upscaler MEMBER video_chroma_upscaler NOTIFY videoChromaUpscalerChanged)
	Q_PROPERTY(VideoRatio video_aspect_ratio MEMBER video_aspect_ratio NOTIFY videoAspectRatioChanged)
	Q_PROPERTY(VideoRatio video_crop_ratio MEMBER video_crop_ratio NOTIFY videoCropRatioChanged)
	Q_PROPERTY(DeintMode video_deinterlacing MEMBER video_deinterlacing NOTIFY videoDeinterlacingChanged)
	Q_PROPERTY(Dithering video_dithering MEMBER video_dithering NOTIFY videoDitheringChanged)
	Q_PROPERTY(StaysOnTop window_stays_on_top MEMBER window_stays_on_top NOTIFY windowStaysOnTopChanged)
	Q_PROPERTY(int play_speed MEMBER play_speed NOTIFY playSpeedChanged)
	Q_PROPERTY(QPoint video_offset MEMBER video_offset NOTIFY videoOffsetChanged)
	Q_PROPERTY(VerticalAlignment video_vertical_alignment MEMBER video_vertical_alignment NOTIFY videoVerticalAlignmentChanged)
	Q_PROPERTY(HorizontalAlignment video_horizontal_alignment MEMBER video_horizontal_alignment NOTIFY videoHorizontalAlignmentChanged)
	Q_PROPERTY(VideoColor video_color MEMBER video_color NOTIFY videoColorChanged)
	Q_PROPERTY(ColorRange video_range MEMBER video_range NOTIFY videoRangeChanged)
	Q_PROPERTY(int audio_volume MEMBER audio_volume NOTIFY audioVolumeChanged)
	Q_PROPERTY(int audio_amp MEMBER audio_amp NOTIFY audioAmpChanged)
	Q_PROPERTY(int audio_sync MEMBER audio_sync NOTIFY audioSyncChanged)
	Q_PROPERTY(bool audio_muted MEMBER audio_muted NOTIFY audioMutedChanged)
	Q_PROPERTY(bool audio_volume_normalizer MEMBER audio_volume_normalizer NOTIFY audioVolumeNormalizerChanged)
	Q_PROPERTY(bool audio_tempo_scaler MEMBER audio_tempo_scaler NOTIFY audioTempoScalerChanged)
	Q_PROPERTY(VerticalAlignment sub_alignment MEMBER sub_alignment NOTIFY subAlignmentChanged)
	Q_PROPERTY(SubtitleDisplay sub_display MEMBER sub_display NOTIFY subDisplayChanged)
	Q_PROPERTY(int sub_position MEMBER sub_position NOTIFY subPositionChanged)
	Q_PROPERTY(int sub_sync MEMBER sub_sync NOTIFY subSyncChanged)
public:
	QPointF win_pos;
	QSize win_size;

// play state
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
	int audio_amp = 100;
	int audio_volume = 100, audio_sync = 0;
	bool audio_muted = false, audio_volume_normalizer = false, audio_tempo_scaler = true;

// subtitle state
	int sub_position = 100;
	int sub_sync = 0;
	SubtitleDisplay sub_display = SubtitleDisplay::OnLetterbox;
	VerticalAlignment sub_alignment = VerticalAlignment::Bottom;

// tool state
	bool auto_exit = false;
	bool playlist_visible = false;
	bool history_visible = false;
	bool playinfo_visible = false;
// window state
	StaysOnTop window_stays_on_top = StaysOnTop::Playing;

// misc
	QString open_last_folder;
	QString open_folder_types = _L("vi");
	QString open_last_file;
	QString open_url_enc;
	QStringList open_url_list;
	bool ask_system_tray = true;

	void save() const;
	static AppState &get();

signals:
	void playSpeedChanged();
	void videoInterpolatorChanged();
	void videoAspectRatioChanged();
	void videoCropRatioChanged();
	void videoDeinterlacingChanged();
	void videoChromaUpscalerChanged();
	void videoDitheringChanged();
	void videoColorChanged();
	void windowStaysOnTopChanged();
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
	void subPositionChanged();
	void subSyncChanged();
	void subDisplayChanged();
	void subAlignmentChanged();
private:
	AppState();
};

#endif // APPSTATE_HPP
