#include "pref.hpp"
#include <QtCore/QMap>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QLocale>

Pref *Pref::obj = 0;

#define PREF_GROUP QLatin1String("preference")

void Pref::save() const {
	Record r(PREF_GROUP);
#define WRITE(a) RECORD_WRITE(r, a)
	WRITE(remember_stopped);
	WRITE(ask_record_found);
	WRITE(pause_minimized);
	WRITE(pause_video_only);
	WRITE(hide_cursor);
	WRITE(hide_cursor_delay);
	WRITE(enable_system_tray);
	WRITE(hide_rather_close);

	WRITE(disable_screensaver);
	WRITE(sub_enc);
	WRITE(sub_priority);
	WRITE(sub_enc_autodetection);
	WRITE(sub_enc_accuracy);
	WRITE(ms_per_char);
	WRITE(seek_step1);
	WRITE(seek_step2);
	WRITE(seek_step3);
	WRITE(speed_step);
	WRITE(volume_step);
	WRITE(amp_step);
	WRITE(sub_pos_step);
	WRITE(volume_step);
	WRITE(sync_delay_step);
	WRITE(brightness_step);
	WRITE(saturation_step);
	WRITE(contrast_step);
	WRITE(hue_step);
	WRITE(locale);
	WRITE(sub_ext);
	WRITE(blur_kern_c);
	WRITE(blur_kern_n);
	WRITE(blur_kern_d);
	WRITE(sharpen_kern_c);
	WRITE(sharpen_kern_n);
	WRITE(sharpen_kern_d);
	WRITE(adjust_contrast_min_luma);
	WRITE(adjust_contrast_max_luma);

	WRITE(enable_generate_playist);
	WRITE(sub_enable_autoload);
	WRITE(sub_enable_autoselect);
	RECORD_WRITE_ENUM(r, generate_playlist);
	RECORD_WRITE_ENUM(r, sub_autoload);
	RECORD_WRITE_ENUM(r, sub_autoselect);

	WRITE(enable_hwaccel);
	WRITE(skin_name);
	r.write("hwaccel_format", _fToString(hwaccel_format));

	sub_style.save(r, "sub_style");
	double_click_map.save(r, "double_click_map");
	middle_click_map.save(r, "middle_click_map");
	wheel_scroll_map.save(r, "wheel_scroll_map");
}

void Pref::load() {
	Record r(PREF_GROUP);
#define READ(a) r.read(a, #a)
#define READ_ENUM(a) r.readEnum(a, #a)
	READ(remember_stopped);
	READ(ask_record_found);
	READ(pause_minimized);
	READ(pause_video_only);
	READ(hide_cursor);
	READ(hide_cursor_delay);
	READ(blur_kern_c);
	READ(blur_kern_n);
	READ(blur_kern_d);
	READ(sharpen_kern_c);
	READ(sharpen_kern_n);
	READ(sharpen_kern_d);
	READ(adjust_contrast_min_luma);
	READ(adjust_contrast_max_luma);

	READ(enable_system_tray);
	READ(hide_rather_close);
	READ(disable_screensaver);
	READ(locale);
	READ(sub_enc);
	READ(sub_enc_autodetection);
	READ(sub_enc_accuracy);
	READ(ms_per_char);
	READ(sub_priority);
	READ(seek_step1);
	READ(seek_step2);
	READ(seek_step3);
	READ(speed_step);
	READ(volume_step);
	READ(amp_step);
	READ(sub_pos_step);
	READ(sync_delay_step);
	READ(brightness_step);
	READ(saturation_step);
	READ(contrast_step);
	READ(hue_step);
	READ(sub_ext);

	READ(skin_name);
	READ(enable_hwaccel);
	hwaccel_format = static_cast<VideoFormat::Type>(_f(r.read("hwaccel_format", _fToString(hwaccel_format))));

	READ(enable_generate_playist);
	READ(sub_enable_autoload);
	READ(sub_enable_autoselect);
	READ_ENUM(generate_playlist);
	READ_ENUM(sub_autoload);
	READ_ENUM(sub_autoselect);
#undef READ
#undef READ_ENUM

	sub_style.load(r, "sub_style");
	double_click_map.load(r, "double_click_map");
	middle_click_map.load(r, "middle_click_map");
	wheel_scroll_map.load(r, "wheel_scroll_map");
}

#undef PREF_GROUP
