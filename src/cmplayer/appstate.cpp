#include "appstate.hpp"
#include "record.hpp"

typedef QLatin1String _LS;

AppState &AppState::get() {
	static AppState obj;
	return obj;
}

AppState::AppState() {
	Record r("app-state");

#define READ(a) RECORD_READ(r, a)
	READ(play_speed);
	READ(video_drop_frame);
	READ(video_aspect_ratio);
	READ(video_crop_ratio);
	RECORD_READ_ENUM(r, video_alignment);
	READ(video_offset)
	READ(video_effects);
	r.read(video_color.brightness(), "video_brightness");
	r.read(video_color.saturation(), "video_saturation");
	r.read(video_color.contrast(), "video_contrast");
	r.read(video_color.hue(), "video_hue");

	READ(audio_volume);
	READ(audio_volume_normalized);
	READ(audio_muted);
	READ(audio_amp);

	READ(sub_pos);
	READ(sub_letterbox);
	READ(sub_align_top);
	READ(sub_sync_delay);

	RECORD_READ_ENUM(r, screen_stays_on_top);

	READ(open_last_file);
	READ(open_url_list);
	READ(open_url_enc);
	READ(ask_system_tray);
#undef READ
}

void AppState::save() const {
	Record r("app-state");
#define WRITE(a) RECORD_WRITE(r, a)
	WRITE(play_speed);

	WRITE(video_drop_frame);
	WRITE(video_aspect_ratio);
	WRITE(video_crop_ratio);
	RECORD_WRITE_ENUM(r, video_alignment);
	WRITE(video_offset);
	WRITE(video_effects)
	r.write("video_brightness", video_color.brightness());
	r.write("video_saturation", video_color.saturation());
	r.write("video_contrast", video_color.contrast());
	r.write("video_hue", video_color.hue());

	WRITE(audio_volume);
	WRITE(audio_volume_normalized);
	WRITE(audio_muted);
	WRITE(audio_amp);

	WRITE(sub_pos);
	WRITE(sub_letterbox);
	WRITE(sub_align_top);
	WRITE(sub_sync_delay);

	RECORD_WRITE_ENUM(r, screen_stays_on_top);

	WRITE(open_last_file);
	WRITE(ask_system_tray);
	WRITE(open_url_list);
	WRITE(open_url_enc);
#undef WRITE
}

