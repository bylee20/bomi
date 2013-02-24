#include "pref.hpp"
#include "hwacc.hpp"

Pref &Pref::get() {
	static Pref pref;
	return pref;
}

#define PREF_GROUP _L("preference")

void Pref::save() const {
	Record r(PREF_GROUP);
#define WRITE(a) r.write(a, #a);
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
	WRITE(generate_playlist);
	WRITE(sub_autoload);
	WRITE(sub_autoselect);

    WRITE(enable_hwaccel);
	WRITE(skin_name);
    WRITE(hwaccel_codecs);

	WRITE(normalizer_silence);
	WRITE(normalizer_target);
	WRITE(normalizer_min);
	WRITE(normalizer_max);
#undef WRITE

#define WRITE2(a) a.save(r, #a);
	WRITE2(open_media_from_file_manager);
	WRITE2(open_media_by_drag_and_drop);
	WRITE2(sub_style);
	WRITE2(double_click_map);
	WRITE2(middle_click_map);
	WRITE2(wheel_scroll_map);
#undef WRITE2
}

void Pref::load() {
	Record r(PREF_GROUP);
#define READ(a) r.read(a, #a)
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
    READ(hwaccel_codecs);

	READ(enable_generate_playist);
	READ(sub_enable_autoload);
	READ(sub_enable_autoselect);
	READ(generate_playlist);
	READ(sub_autoload);
	READ(sub_autoselect);

	READ(normalizer_silence);
	READ(normalizer_target);
	READ(normalizer_min);
	READ(normalizer_max);
#undef READ
//	sub_style.load(r, "sub_style");
	double_click_map.load(r, "double_click_map");
	middle_click_map.load(r, "middle_click_map");
	wheel_scroll_map.load(r, "wheel_scroll_map");
}

QList<int> Pref::defaultHwAccCodecs() {
	QList<int> codecs;
	for (auto codec : HwAcc::fullCodecList()) {
		if (HwAcc::supports(codec))
			codecs.push_back(codec);
	}
	return codecs;
}

#undef PREF_GROUP
