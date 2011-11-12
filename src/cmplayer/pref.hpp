#ifndef PREF_HPP
#define PREF_HPP

#include <QtCore/QString>
#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtCore/QMap>
#include "global.hpp"
#include "enums.hpp"
#include "record.hpp"
#include "osdstyle.hpp"

class QLocale;

class Pref {
public:
	static const Pref &get() {Q_ASSERT(obj != 0); return *obj;}
	template <typename Enum>
	struct ActionEnumInfo {
		typedef Enum EnumType;
		ActionEnumInfo(): enabled(false) {}
		ActionEnumInfo(bool e, const Enum &a): enabled(e), action(a) {}
		bool enabled; Enum action;
	};
	typedef ActionEnumInfo<Enum::ClickAction> ClickActionInfo;
	typedef ActionEnumInfo<Enum::WheelAction> WheelActionInfo;
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
				r.write("enabled", info.enabled);
				r.writeEnum("action", info.action);
				r.endGroup();
			}
			r.endGroup();
		}
		void load(Record &r, const QString &group, const KeyModifierMap &def) {
			r.beginGroup(group);
			const Modifier::List &list = Modifier::list();
			for (int i=0; i<list.size(); ++i) {
				InfoType &info = m_map[list[i].id()];
				const InfoType &d = def[list[i]];
				r.beginGroup(list[i].name());
				info.enabled = r.read("enabled", d.enabled);
				info.action = r.readEnum("action", d.action);
				r.endGroup();
			}
			r.endGroup();
		}
	private:
		Map m_map;
	};
	typedef KeyModifierMap<Enum::ClickAction> ClickActionMap;
	typedef KeyModifierMap<Enum::WheelAction> WheelActionMap;

	bool pause_minimized, pause_video_only;
	bool remember_stopped, ask_record_found;
	Enum::GeneratePlaylist generate_playlist;
	bool hide_cursor, disable_screensaver;
	int hide_cursor_delay;

	int blur_kern_c, blur_kern_n, blur_kern_d;
	int sharpen_kern_c, sharpen_kern_n, sharpen_kern_d;
	int adjust_contrast_min_luma, adjust_contrast_max_luma;
	double auto_contrast_threshold;

	int normalizer_gain, normalizer_smoothness;

	Enum::SubtitleAutoload sub_autoload;
	Enum::SubtitleAutoselect sub_autoselect;
	QString sub_enc, sub_ext;
	bool sub_enc_autodetection;
	int sub_enc_accuracy;
	OsdStyle sub_style;
	int ms_per_char;
	QStringList sub_priority;

	QLocale locale;
	bool enable_system_tray, hide_rather_close;
	ClickActionMap double_click_map, middle_click_map;
	WheelActionMap wheel_scroll_map;
	int seek_step1, seek_step2, seek_step3, speed_step;
	int brightness_step, saturation_step, contrast_step, hue_step;
	int volume_step, sync_delay_step, amp_step, sub_pos_step;

	void save() const;
	void load();
	class Dialog;
	class MacDialog;
	class Widget;
private:
	static Pref *obj;
	Pref() {load();}
	friend int main(int, char**);
};


#endif // PREF_HPP
