#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "enums.hpp"
#include <QtCore/QStringList>
#include <QtCore/QPointF>
#include "colorproperty.hpp"

class AppState {
public:
// play state
	double play_speed = 1.0;

// video state
	bool video_drop_frame = false;
	double video_aspect_ratio = -1.0, video_crop_ratio = -1.0;
	int video_effects = 0;
	ColorProperty video_color = {0.0, 0.0, 0.0, 0.0};
	Enum::Position video_alignment = Enum::Position::CC;
	QPoint video_offset = {0, 0};

// audio state
	double audio_amp = 1.0;
	int audio_volume = 100;
	bool audio_muted = false, audio_volume_normalized = true;

// subtitle state
	double sub_pos = 1.0;
	int sub_sync_delay = 0;
	bool sub_letterbox = true, sub_align_top = false;

// window state
	Enum::StaysOnTop screen_stays_on_top = Enum::StaysOnTop::Playing;

// misc
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
