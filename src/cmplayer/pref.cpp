#include "pref.hpp"
#include "hwacc.hpp"

template<typename T>
static QStringList toStringList(const QList<T> &list) {
	QStringList ret;
	ret.reserve(list.size());
	for (int i=0; i<list.size(); ++i)
		ret.push_back(list[i].toString());
	return ret;
}

template<typename T>
static QList<T> fromStringList(const QStringList &list) {
	QList<T> ret;
	ret.reserve(list.size());
	for (int i=0; i<list.size(); ++i)
		ret.push_back(T::fromString(list[i]));
	return ret;
}


QHash<QString, QList<QKeySequence> > Pref::defaultShortcuts() {
	QHash<QString, QList<QKeySequence> > keys;
	keys[_L("menu/open/file")] << Qt::CTRL + Qt::Key_F;
	keys[_L("menu/open/folder")] << Qt::CTRL + Qt::Key_G;

	keys[_L("menu/play/pause")] << Qt::Key_Space;
	keys[_L("menu/play/prev")] << Qt::CTRL + Qt::Key_Left;
	keys[_L("menu/play/next")] << Qt::CTRL + Qt::Key_Right;
	keys[_L("menu/play/speed/slower")] << Qt::Key_Minus;
	keys[_L("menu/play/speed/reset")] << Qt::Key_Backspace;
	keys[_L("menu/play/speed/faster")] << Qt::Key_Plus << Qt::Key_Equal;
	keys[_L("menu/play/repeat/range")] << Qt::Key_R;
	keys[_L("menu/play/repeat/subtitle")] << Qt::Key_E;
	keys[_L("menu/play/repeat/quit")] << Qt::Key_Escape;
	keys[_L("menu/play/seek/forward1")] << Qt::Key_Right;
	keys[_L("menu/play/seek/forward2")] << Qt::Key_PageDown;
	keys[_L("menu/play/seek/forward3")] << Qt::Key_End;
	keys[_L("menu/play/seek/backward1")] << Qt::Key_Left;
	keys[_L("menu/play/seek/backward2")] << Qt::Key_PageUp;
	keys[_L("menu/play/seek/backward3")] << Qt::Key_Home;
	keys[_L("menu/play/seek/prev-subtitle")] << Qt::Key_Comma;
	keys[_L("menu/play/seek/current-subtitle")] << Qt::Key_Period;
	keys[_L("menu/play/seek/next-subtitle")] << Qt::Key_Slash;

	keys[_L("menu/subtitle/track/next")] << Qt::CTRL + Qt::Key_E;
	keys[_L("menu/subtitle/list/open")] << Qt::SHIFT + Qt::Key_F;
	keys[_L("menu/subtitle/list/next")] << Qt::SHIFT + Qt::Key_E;
	keys[_L("menu/subtitle/list/all")] << Qt::SHIFT + Qt::Key_R;
	keys[_L("menu/subtitle/list/hide")] << Qt::SHIFT + Qt::Key_Q;
	keys[_L("menu/subtitle/pos-up")] << Qt::Key_W;
	keys[_L("menu/subtitle/pos-down")] << Qt::Key_S;
	keys[_L("menu/subtitle/sync-add")] << Qt::Key_D;
	keys[_L("menu/subtitle/sync-reset")] << Qt::Key_Q;
	keys[_L("menu/subtitle/sync-sub")] << Qt::Key_A;

	keys[_L("menu/video/snapshot")] << Qt::CTRL + Qt::Key_S;
	keys[_L("menu/video/drop-frame")] << Qt::CTRL + Qt::Key_D;
	keys[_L("menu/video/move/reset")] << Qt::SHIFT + Qt::Key_X;
	keys[_L("menu/video/move/up")] << Qt::SHIFT + Qt::Key_W;
	keys[_L("menu/video/move/down")] << Qt::SHIFT + Qt::Key_S;
	keys[_L("menu/video/move/left")] << Qt::SHIFT + Qt::Key_A;
	keys[_L("menu/video/move/right")] << Qt::SHIFT + Qt::Key_D;
	keys[_L("menu/video/color/brightness+")] << Qt::Key_T;
	keys[_L("menu/video/color/brightness-")] << Qt::Key_G;
	keys[_L("menu/video/color/contrast+")] << Qt::Key_Y;
	keys[_L("menu/video/color/contrast-")] << Qt::Key_H;
	keys[_L("menu/video/color/saturation+")] << Qt::Key_U;
	keys[_L("menu/video/color/saturation-")] << Qt::Key_J;
	keys[_L("menu/video/color/hue+")] << Qt::Key_I;
	keys[_L("menu/video/color/hue-")] << Qt::Key_K;

	keys[_L("menu/audio/track/next")] << Qt::CTRL + Qt::Key_A;
	keys[_L("menu/audio/volume-up")] << Qt::Key_Up;
	keys[_L("menu/audio/volume-down")] << Qt::Key_Down;
	keys[_L("menu/audio/mute")] << Qt::Key_M;
	keys[_L("menu/audio/normalizer")] << Qt::Key_N;
	keys[_L("menu/audio/tempo-scaler")] << Qt::Key_Z;
	keys[_L("menu/audio/amp-up")] << Qt::CTRL + Qt::Key_Up;
	keys[_L("menu/audio/amp-down")] << Qt::CTRL + Qt::Key_Down;
	keys[_L("menu/audio/sync-reset")] << Qt::Key_Backslash;
	keys[_L("menu/audio/sync-add")] << Qt::Key_BracketRight;
	keys[_L("menu/audio/sync-sub")] << Qt::Key_BracketLeft;

	keys[_L("menu/tool/playlist/toggle")] << Qt::Key_L;
	keys[_L("menu/tool/history")] << Qt::Key_C;
	keys[_L("menu/tool/subtitle")] << Qt::Key_V;
	keys[_L("menu/tool/pref")] << Qt::Key_P;
	keys[_L("menu/tool/reload-skin")] << Qt::Key_R + Qt::CTRL;
	keys[_L("menu/tool/playinfo")] << Qt::Key_Tab;

	keys[_L("menu/window/proper")] << Qt::Key_QuoteLeft;
	keys[_L("menu/window/100%")] << Qt::Key_1;
	keys[_L("menu/window/200%")] << Qt::Key_2;
	keys[_L("menu/window/300%")] << Qt::Key_3;
	keys[_L("menu/window/400%")] << Qt::Key_4;
	keys[_L("menu/window/full")] << Qt::Key_Enter << Qt::Key_Return << Qt::Key_F;
	keys[_L("menu/window/close")] << Qt::CTRL + Qt::Key_W;

#ifndef Q_OS_MAC
	keys[_L("menu/exit")] << Qt::CTRL + Qt::Key_Q;
#endif
	return keys;
}

Shortcuts Pref::preset(ShortcutPreset id) {
	Shortcuts keys;
	if (id == Movist) {
		keys[_L("menu/open/file")] << Qt::CTRL + Qt::Key_O;
		keys[_L("menu/window/close")] << Qt::CTRL + Qt::Key_W;
		keys[_L("menu/tool/playlist/save")] << Qt::CTRL + Qt::Key_S << Qt::CTRL + Qt::SHIFT + Qt::Key_S;
		keys[_L("menu/tool/playlist/append-file")] << Qt::CTRL + Qt::ALT + Qt::Key_L;
		keys[_L("menu/play/prev")] << Qt::ALT + Qt::CTRL + Qt::Key_Left;
		keys[_L("menu/play/next")] << Qt::ALT + Qt::CTRL + Qt::Key_Right;
		keys[_L("menu/play/pause")] << Qt::Key_Space;
		keys[_L("menu/play/seek/backword1")] << Qt::Key_Left;
		keys[_L("menu/play/seek/forward1")] << Qt::Key_Right;
		keys[_L("menu/play/repeat/quit")] << Qt::CTRL + Qt::Key_Backslash;
		keys[_L("menu/play/seek/range")] << Qt::CTRL + Qt::Key_BracketLeft << Qt::CTRL + Qt::Key_BracketRight;
		keys[_L("menu/play/speed/reset")] << Qt::SHIFT + Qt::CTRL + Qt::Key_Backslash;
		keys[_L("menu/play/repeat/faster")] << Qt::SHIFT + Qt::CTRL + Qt::Key_Right;
		keys[_L("menu/play/speed/slower")] << Qt::SHIFT + Qt::CTRL + Qt::Key_Left;
		keys[_L("menu/window/full")] << Qt::META + Qt::CTRL + Qt::Key_F;
		keys[_L("menu/window/100%")] << Qt::CTRL + Qt::Key_1;
		keys[_L("menu/window/200%")] << Qt::CTRL + Qt::Key_2;
		keys[_L("menu/audio/track/next")] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_S;
		keys[_L("menu/audio/sync-reset")] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_Backslash;
		keys[_L("menu/audio/sync-add")] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_Right;
		keys[_L("menu/audio/sync-sub")] << Qt::META + Qt::ALT + Qt::CTRL + Qt::Key_Left;
		keys[_L("menu/audio/volume-up")] << Qt::ALT + Qt::Key_Up << Qt::Key_Up;
		keys[_L("menu/audio/volume-down")] << Qt::ALT + Qt::Key_Down << Qt::Key_Down;
		keys[_L("menu/audio/mute")] << Qt::ALT + Qt::CTRL + Qt::Key_Down;
		keys[_L("menu/subtitle/list/next")] << Qt::META + Qt::CTRL + Qt::Key_S;
		keys[_L("menu/subtitle/list/hide")] << Qt::META + Qt::CTRL + Qt::Key_V;
		keys[_L("menu/subtitle/sync-reset")] << Qt::META + Qt::SHIFT + Qt::Key_Equal;
		keys[_L("menu/subtitle/sync-add")] << Qt::META + Qt::SHIFT + Qt::Key_Right;
		keys[_L("menu/subtitle/sync-sub")] << Qt::META + Qt::SHIFT + Qt::Key_Left;
		keys[_L("menu/window/sot-always")] << Qt::CTRL + Qt::Key_T;
		keys[_L("menu/window/sot-playing")] << Qt::CTRL + Qt::ALT + Qt::Key_T;
		keys[_L("menu/window/minimize")] << Qt::ALT + Qt::CTRL + Qt::Key_M;
		keys[_L("menu/tool/playlist/toggle")] << Qt::CTRL + Qt::Key_L;
		keys[_L("menu/tool/playinfo")] << Qt::CTRL + Qt::Key_P;
	} else
		keys = defaultShortcuts();
	return keys;
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
	WRITE(sub_sync_step);
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
	WRITE(remap_luma_min);
	WRITE(remap_luma_max);

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

	WRITE(lion_style_fullscreen);
#undef WRITE

#define WRITE2(a) a.save(r, #a);
	WRITE2(open_media_from_file_manager);
	WRITE2(open_media_by_drag_and_drop);
	WRITE2(sub_style);
	WRITE2(double_click_map);
	WRITE2(middle_click_map);
	WRITE2(wheel_scroll_map);
#undef WRITE2

	r.beginWriteArray("shortcuts", shortcuts.size());
	auto it = shortcuts.cbegin();
	for (int i=0; it != shortcuts.cend(); ++it, ++i) {
		r.setArrayIndex(i);
		r.setValue("id", it.key());
		r.setValue("keys", toStringList(it.value()));
	}
	r.endArray();
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
	READ(remap_luma_min);
	READ(remap_luma_max);

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
	READ(sub_sync_step);
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

	READ(lion_style_fullscreen);
#undef READ

#define READ2(a) a.load(r, #a)
	READ2(open_media_from_file_manager);
	READ2(open_media_by_drag_and_drop);
	READ2(sub_style);
	READ2(double_click_map);
	READ2(middle_click_map);
	READ2(wheel_scroll_map);
#undef READ2

	const auto size = r.beginReadArray("shortcuts");
	if (size > 0) {
		shortcuts.clear();
		for (int i=0; i<size; ++i) {
			r.setArrayIndex(i);
			const auto id = r.value("id").toString();
			if (!id.isEmpty()) {
				const auto keys = fromStringList<QKeySequence>(r.value("keys").toStringList());
				if (!keys.isEmpty())
					shortcuts[id] = keys;
			}
		}
	}
	r.endArray();
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
