#ifndef PREF_HPP
#define PREF_HPP

#include "stdafx.hpp"
#include "global.hpp"
#include "enums.hpp"
#include "record.hpp"
#include "subtitlestyle.hpp"

class QLocale;

typedef QHash<QString, QList<QKeySequence>> Shortcuts;

template <typename Enum>
struct ActionEnumInfo {
	typedef Enum EnumType;
	typedef ActionEnumInfo<Enum> Super ;
	ActionEnumInfo(): enabled(false) {}
	ActionEnumInfo(bool e, const Enum &a): enabled(e), action(a) {}
	bool enabled; Enum action;
};
using ClickActionInfo = ActionEnumInfo<Enum::ClickAction>;
using WheelActionInfo = ActionEnumInfo<Enum::WheelAction>;

class Pref {
public:
	enum ShortcutPreset {CMPlayer, Movist};
//	static const Pref &instance() {return get();}
	template <typename Enum>
	struct KeyModifierMap {
		typedef ::Enum::KeyModifier Modifier;
		typedef ActionEnumInfo<Enum> InfoType;
		typedef QMap<int, InfoType> Map;
		KeyModifierMap(bool enabled = false, const Enum &e = Enum()) {
			const InfoType def(enabled, e);
			const Modifier::List &list = Modifier::list();
			for (int i=0; i<list.size(); ++i) m_map[list[i].id()] = def;
		}
		InfoType &operator[](const Modifier &m) {return m_map[m.id()];}
		const InfoType operator[](const Modifier &m) const {return m_map[m.id()];}
		const InfoType operator[](int id) const {return m_map.value(id);}
		void save(Record &r, const QString &group) const {
			r.beginGroup(group);
			const Modifier::List &list = Modifier::list();
			for (int i=0; i<list.size(); ++i) {
				const InfoType &info = m_map[list[i].id()];
				r.beginGroup(list[i].name());
				r.write(info.enabled, "enabled");
				r.write(info.action, "action");
				r.endGroup();
			}
			r.endGroup();
		}
		void load(Record &r, const QString &group) {
			r.beginGroup(group);
			const Modifier::List &list = Modifier::list();
			for (int i=0; i<list.size(); ++i) {
				InfoType &info = m_map[list[i].id()];
				r.beginGroup(list[i].name());
				r.read(info.enabled, "enabled");
				r.read(info.action, "action");
				r.endGroup();
			}
			r.endGroup();
		}
	private:
		Map m_map;
	};

	struct OpenMedia {
		OpenMedia(bool sp, const Enum::PlaylistBehaviorWhenOpenMedia &pb)
		: start_playback(sp), playlist_behavior(pb) {}
		bool start_playback = true;
		Enum::PlaylistBehaviorWhenOpenMedia playlist_behavior = Enum::PlaylistBehaviorWhenOpenMedia::ClearAndGenerateNewPlaylist;
		void save(Record &r, const QString &group) const {
			r.beginGroup(group);
			r.write(start_playback, "start_playback");
			r.write(playlist_behavior, "playlist_behavior");
			r.endGroup();
		}
		void load(Record &r, const QString &group) {
			r.beginGroup(group);
			r.read(start_playback, "start_playback");
			r.read(playlist_behavior, "playlist_behavior");
			r.endGroup();
		}
	};

	typedef KeyModifierMap<Enum::ClickAction> ClickActionMap;
	typedef KeyModifierMap<Enum::WheelAction> WheelActionMap;

	OpenMedia open_media_from_file_manager = {true, Enum::PlaylistBehaviorWhenOpenMedia::ClearAndAppendToPlaylist};
	OpenMedia open_media_by_drag_and_drop = {true, Enum::PlaylistBehaviorWhenOpenMedia::AppendToPlaylist};

	bool pause_minimized = true, pause_video_only = true, pause_to_play_next_image = true;
	bool remember_stopped = true, ask_record_found = true, remember_image = false;
	bool enable_generate_playist = true;
	Enum::GeneratePlaylist generate_playlist = Enum::GeneratePlaylist::Folder;
	bool hide_cursor = true, disable_screensaver = true, lion_style_fullscreen = false;
	int hide_cursor_delay = 3000, image_duration = 0;
	bool show_osd_on_action = true, show_osd_on_resized = true;
	bool show_logo = true; QColor bg_color = Qt::black;
	int blur_kern_c = 1, blur_kern_n = 2, blur_kern_d = 1;
	int sharpen_kern_c = 5, sharpen_kern_n = -1, sharpen_kern_d = 0;
	int remap_luma_min = 16, remap_luma_max = 235;

	bool sub_enable_autoload = true, sub_enable_autoselect = true, sub_enc_autodetection = true;
	Enum::SubtitleAutoload sub_autoload = Enum::SubtitleAutoload::Contain;
	Enum::SubtitleAutoselect sub_autoselect = Enum::SubtitleAutoselect::Matched;
	QString sub_enc = {QLocale::system().language() == QLocale::Korean ? "CP949" : "UTF-8"}, sub_ext;
	int sub_enc_accuracy = 70, ms_per_char = 500;
	SubtitleStyle sub_style;		QStringList sub_priority;

	QLocale locale = QLocale::system();
	bool enable_system_tray = true, hide_rather_close = true;
	ClickActionMap double_click_map = defaultDoubleClick(), middle_click_map = defaultMiddleClick();
	WheelActionMap wheel_scroll_map = defaultWheelAction();
	int seek_step1 = 5000, seek_step2 = 30000, seek_step3 = 60000, speed_step = 10;
	int brightness_step = 1, saturation_step = 1, contrast_step = 1, hue_step = 1;
	int volume_step = 2, sub_sync_step = 500, amp_step = 10, sub_pos_step = 1, audio_sync_step = 200;

    bool enable_hwaccel = false;
    QList<int> hwaccel_codecs = defaultHwAccCodecs();

	double normalizer_silence = 0.0001, normalizer_target = 0.25, normalizer_min = 0.1, normalizer_max = 10.0;

	QString skin_name = "modern";

	Shortcuts shortcuts = defaultShortcuts();

	static Shortcuts preset(ShortcutPreset id);

	void save() const;
	void load();
private:
//	static Pref &get();
	static QList<int> defaultHwAccCodecs();
	static Shortcuts defaultShortcuts();
	static ClickActionMap defaultDoubleClick() {
		ClickActionMap map = {false, Enum::ClickAction::Fullscreen};
		map[Enum::KeyModifier::None].enabled = true;
		return map;
	}
	static ClickActionMap defaultMiddleClick() {
		ClickActionMap map = {false, Enum::ClickAction::Fullscreen};
		map[Enum::KeyModifier::None] = ClickActionInfo(true, Enum::ClickAction::Pause);
		return map;
	}
	static WheelActionMap defaultWheelAction() {
		WheelActionMap map(false, Enum::WheelAction::Volume);
		map[Enum::KeyModifier::None].enabled = true;
		map[Enum::KeyModifier::Ctrl] = WheelActionInfo(true, Enum::WheelAction::Amp);
		return map;
	}
};

//#define cPref (Pref::instance())

#endif // PREF_HPP
