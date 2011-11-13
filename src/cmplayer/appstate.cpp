#include "appstate.hpp"
#include "record.hpp"

typedef QLatin1String _LS;

AppState &AppState::get() {
	static AppState obj;
	return obj;
}

AppState::AppState() {
	Record r("app-state");

	RECORD_READ(r, speed, 1.0);

	RECORD_READ(r, aspect_ratio, -1.0);
	RECORD_READ(r, crop_ratio, -1.0);
	RECORD_READ_ENUM(r, screen_alignment, Enum::Position::CC);
	RECORD_READ(r, screen_offset, QPoint(0, 0));
	RECORD_READ(r, video_effects, 0);
	video_color.brightness() = r.read("video_brightness", 0.0);
	video_color.saturation() = r.read("video_saturation", 0.0);
	video_color.contrast() = r.read("video_contrast", 0.0);
	video_color.hue() = r.read("video_hue", 0.0);
	RECORD_READ_ENUM(r, overlay, Enum::Overlay::Auto);

	RECORD_READ(r, volume, 100);
	RECORD_READ(r, volume_normalized, true);
	RECORD_READ(r, muted, false);
	RECORD_READ(r, amp, 1.0);

	RECORD_READ(r, sub_pos, 1.0);
	RECORD_READ(r, sub_letterbox, true);
	RECORD_READ(r, sub_align_top, false);
	RECORD_READ(r, sub_sync_delay, 0);

	RECORD_READ_ENUM(r, stays_on_top, Enum::StaysOnTop::Playing);

	RECORD_READ(r, last_open_file, QString());
	RECORD_READ(r, ask_system_tray, true);
	RECORD_READ(r, open_url_list, QStringList());
	RECORD_READ(r, url_enc, QString());

}

void AppState::save() const {
	Record r("app-state");

	RECORD_WRITE(r, speed);

	RECORD_WRITE(r, aspect_ratio);
	RECORD_WRITE(r, crop_ratio);
	RECORD_WRITE_ENUM(r, screen_alignment);
	RECORD_WRITE(r, screen_offset);
	RECORD_WRITE(r, video_effects)
	r.write("video_brightness", video_color.brightness());
	r.write("video_saturation", video_color.saturation());
	r.write("video_contrast", video_color.contrast());
	r.write("video_hue", video_color.hue());
	RECORD_WRITE_ENUM(r, overlay);

	RECORD_WRITE(r, volume);
	RECORD_WRITE(r, volume_normalized);
	RECORD_WRITE(r, muted);
	RECORD_WRITE(r, amp);

	RECORD_WRITE(r, sub_pos);
	RECORD_WRITE(r, sub_letterbox);
	RECORD_WRITE(r, sub_align_top);
	RECORD_WRITE(r, sub_sync_delay);

	RECORD_WRITE_ENUM(r, stays_on_top);

	RECORD_WRITE(r, last_open_file);
	RECORD_WRITE(r, ask_system_tray);
	RECORD_WRITE(r, open_url_list);
	RECORD_WRITE(r, url_enc);
}

