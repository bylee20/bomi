#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "stdafx.hpp"
#include "enums.hpp"
#include "colorproperty.hpp"

class AppState {
public:
	QPointF win_pos;
	QSize win_size;

// play state
	double play_speed = 1.0;

// video state
	bool video_drop_frame = false;
	double video_aspect_ratio = -1.0, video_crop_ratio = -1.0;
	int video_effects = 0;
	ColorProperty video_color = {0.0, 0.0, 0.0, 0.0};
	Position video_alignment = Position::CC;
	QPoint video_offset = {0, 0};
	DeintMode video_deint = DeintMode::Auto;
	InterpolatorType video_interpolator = InterpolatorType::Bilinear;
// audio state
	double audio_preamp = 1.0;
	int audio_volume = 100, audio_sync = 0;
	bool audio_muted = false, audio_normalizer = false, audio_scaletempo = true;

// subtitle state
	double sub_pos = 1.0;
	int sub_sync_delay = 0;
	bool sub_letterbox = true, sub_align_top = false;

// tool state
	bool auto_exit = false;
	bool playlist_visible = false;
	bool history_visible = false;
	bool playinfo_visible = false;
// window state
	StaysOnTop screen_stays_on_top = StaysOnTop::Playing;

// misc
	QString open_last_folder;
	QString open_folder_types = _L("vi");
	QString open_last_file;
	QString open_url_enc;
	QStringList open_url_list;
	bool ask_system_tray = true;

	void save() const;
	static AppState &get();
private:
	AppState();
};

#endif // APPSTATE_HPP
