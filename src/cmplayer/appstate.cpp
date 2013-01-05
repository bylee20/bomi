#include "appstate.hpp"
#include "record.hpp"

typedef QLatin1String _L;

AppState &AppState::get() {
	static AppState obj;
	return obj;
}

AppState::AppState() {
	Record r("app-state");

#define READ(a) r.read(a, #a)
	READ(play_speed);
	READ(video_drop_frame);
	READ(video_aspect_ratio);
	READ(video_crop_ratio);
	READ(video_alignment);
	READ(video_offset);
	READ(video_effects);
	READ(video_color);

	READ(audio_volume);
	READ(audio_volume_normalized);
	READ(audio_muted);
	READ(audio_amp);

	READ(sub_pos);
	READ(sub_letterbox);
	READ(sub_align_top);
	READ(sub_sync_delay);

	READ(screen_stays_on_top);

	READ(open_last_file);
	READ(open_url_list);
	READ(open_url_enc);
	READ(ask_system_tray);

	READ(auto_exit);
#undef READ
}

void AppState::save() const {
	Record r("app-state");
#define WRITE(a) r.write(a, #a);
	WRITE(play_speed);

	WRITE(video_drop_frame);
	WRITE(video_aspect_ratio);
	WRITE(video_crop_ratio);
	WRITE(video_alignment);
	WRITE(video_offset);
	WRITE(video_effects);
	WRITE(video_color);

	WRITE(audio_volume);
	WRITE(audio_volume_normalized);
	WRITE(audio_muted);
	WRITE(audio_amp);

	WRITE(sub_pos);
	WRITE(sub_letterbox);
	WRITE(sub_align_top);
	WRITE(sub_sync_delay);

	WRITE(screen_stays_on_top);

	WRITE(open_last_file);
	WRITE(ask_system_tray);
	WRITE(open_url_list);
	WRITE(open_url_enc);

	WRITE(auto_exit);
#undef WRITE
}

