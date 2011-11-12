#include "pref.hpp"
#include <QtCore/QMap>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QLocale>

Pref *Pref::obj = 0;

#define PREF_GROUP QLatin1String("preference")

void Pref::save() const {
	Record r(PREF_GROUP);

	RECORD_WRITE(r, remember_stopped);
	RECORD_WRITE(r, ask_record_found);
	RECORD_WRITE(r, pause_minimized);
	RECORD_WRITE(r, pause_video_only);
	RECORD_WRITE(r, hide_cursor);
	RECORD_WRITE(r, hide_cursor_delay);
	RECORD_WRITE(r, enable_system_tray);
	RECORD_WRITE(r, hide_rather_close);

	RECORD_WRITE(r, disable_screensaver);
	RECORD_WRITE(r, sub_enc);
	RECORD_WRITE(r, sub_priority);
	RECORD_WRITE(r, sub_enc_autodetection);
	RECORD_WRITE(r, sub_enc_accuracy);
	RECORD_WRITE(r, ms_per_char);
	RECORD_WRITE(r, seek_step1);
	RECORD_WRITE(r, seek_step2);
	RECORD_WRITE(r, seek_step3);
	RECORD_WRITE(r, speed_step);
	RECORD_WRITE(r, volume_step);
	RECORD_WRITE(r, amp_step);
	RECORD_WRITE(r, sub_pos_step);
	RECORD_WRITE(r, volume_step);
	RECORD_WRITE(r, sync_delay_step);
	RECORD_WRITE(r, brightness_step);
	RECORD_WRITE(r, saturation_step);
	RECORD_WRITE(r, contrast_step);
	RECORD_WRITE(r, hue_step);
	RECORD_WRITE(r, locale);
	RECORD_WRITE(r, sub_ext);
	RECORD_WRITE(r, blur_kern_c);
	RECORD_WRITE(r, blur_kern_n);
	RECORD_WRITE(r, blur_kern_d);
	RECORD_WRITE(r, sharpen_kern_c);
	RECORD_WRITE(r, sharpen_kern_n);
	RECORD_WRITE(r, sharpen_kern_d);
	RECORD_WRITE(r, adjust_contrast_min_luma);
	RECORD_WRITE(r, adjust_contrast_max_luma);
	RECORD_WRITE(r, auto_contrast_threshold);
	RECORD_WRITE(r, normalizer_gain);
	RECORD_WRITE(r, normalizer_smoothness);

	RECORD_WRITE_ENUM(r, generate_playlist);
	RECORD_WRITE_ENUM(r, sub_autoload);
	RECORD_WRITE_ENUM(r, sub_autoselect);

	sub_style.save(r, "sub_style");
	double_click_map.save(r, "double_click_map");
	middle_click_map.save(r, "middle_click_map");
	wheel_scroll_map.save(r, "wheel_scroll_map");
}

void Pref::load() {
	const int DefaultSeekingStep1 = 5000;
	const int DefaultSeekingStep2 = 30000;
	const int DefaultSeekingStep3 = 60000;
	const int DefaultVolumeStep = 2;
	const int DefaultSyncDelayStep = 500;
	const int DefaultAmpStep = 10;
	const int DefaultSubPosStep = 1;
	const int DefaultSpeedStep = 10;
	const int DefaultColorPropStep = 1;

	Record r(PREF_GROUP);

	RECORD_READ(r, remember_stopped, true);
	RECORD_READ(r, ask_record_found, true);
	RECORD_READ(r, pause_minimized, true);
	RECORD_READ(r, pause_video_only, true);
	RECORD_READ(r, hide_cursor, true);
	RECORD_READ(r, hide_cursor_delay, 3000);
	RECORD_READ(r, blur_kern_c, 1);
	RECORD_READ(r, blur_kern_n, 2);
	RECORD_READ(r, blur_kern_d, 1);
	RECORD_READ(r, sharpen_kern_c, 5);
	RECORD_READ(r, sharpen_kern_n, -1);
	RECORD_READ(r, sharpen_kern_d, 0);
	RECORD_READ(r, adjust_contrast_min_luma, 16);
	RECORD_READ(r, adjust_contrast_max_luma, 235);
	RECORD_READ(r, auto_contrast_threshold, 0.5);

	RECORD_READ(r, enable_system_tray, true);
	RECORD_READ(r, hide_rather_close, true);
	RECORD_READ(r, disable_screensaver, true);
	RECORD_READ(r, locale, QLocale::system());
	RECORD_READ(r, sub_enc, QString(locale.language() == QLocale::Korean ? "CP949" : "UTF-8"));
	RECORD_READ(r, sub_enc_autodetection, true);
	RECORD_READ(r, sub_enc_accuracy, 70);
	RECORD_READ(r, ms_per_char, 500);
	RECORD_READ(r, sub_priority, QStringList());
	RECORD_READ(r, seek_step1, DefaultSeekingStep1);
	RECORD_READ(r, seek_step2, DefaultSeekingStep2);
	RECORD_READ(r, seek_step3, DefaultSeekingStep3);
	RECORD_READ(r, speed_step, DefaultSpeedStep);
	RECORD_READ(r, volume_step, DefaultVolumeStep);
	RECORD_READ(r, amp_step, DefaultAmpStep);
	RECORD_READ(r, sub_pos_step, DefaultSubPosStep);
	RECORD_READ(r, sync_delay_step, DefaultSyncDelayStep);
	RECORD_READ(r, brightness_step, DefaultColorPropStep);
	RECORD_READ(r, saturation_step, DefaultColorPropStep);
	RECORD_READ(r, contrast_step, DefaultColorPropStep);
	RECORD_READ(r, hue_step, DefaultColorPropStep);
	RECORD_READ(r, sub_ext, QString());
	RECORD_READ(r, normalizer_gain, 20);
	RECORD_READ(r, normalizer_smoothness, 100);

	RECORD_READ_ENUM(r, generate_playlist, Enum::GeneratePlaylist::Folder);
	RECORD_READ_ENUM(r, sub_autoload, Enum::SubtitleAutoload::Contain);
	RECORD_READ_ENUM(r, sub_autoselect, Enum::SubtitleAutoselect::Matched);

	sub_style.border_width = 0.045;
	sub_style.text_scale = 0.040;
	sub_style.auto_size = OsdStyle::AutoSize::Width;
	sub_style.has_shadow = true;
	sub_style.shadow_color = Qt::black;
	sub_style.shadow_offset = QPointF(0, 0);
	sub_style.shadow_blur = 3;
	sub_style.font.setBold(true);
	sub_style.load(r, "sub_style");

	ClickActionMap def_click(false, Enum::ClickAction::Fullscreen);
	def_click[Enum::KeyModifier::None].enabled = true;
	double_click_map.load(r, "double_click_map", def_click);
	def_click = ClickActionMap(false, Enum::ClickAction::Fullscreen);
	def_click[Enum::KeyModifier::None] = ClickActionInfo(true, Enum::ClickAction::Pause);
	middle_click_map.load(r, "middle_click_map", def_click);

	WheelActionMap def_wheel(false, Enum::WheelAction::Volume);
	def_wheel[Enum::KeyModifier::None].enabled = true;
	def_wheel[Enum::KeyModifier::Ctrl] = WheelActionInfo(true, Enum::WheelAction::Amp);
	wheel_scroll_map.load(r, "wheel_scroll_map", def_wheel);
}

#undef PREF_GROUP
