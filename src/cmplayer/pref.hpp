#ifndef PREF_HPP
#define PREF_HPP

#include <QtCore/QString>
#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtCore/QMap>
#include "global.hpp"
#include "enums.hpp"
#include "avmisc.hpp"
#include "record.hpp"
#include "osdstyle.hpp"

class QLocale;

class Pref {
public:
	static const Pref &get() {return *obj;}
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
		void load(Record &r, const QString &group) {
			r.beginGroup(group);
			const Modifier::List &list = Modifier::list();
			for (int i=0; i<list.size(); ++i) {
				InfoType &info = m_map[list[i].id()];
				r.beginGroup(list[i].name());
				r.read(info.enabled, "enabled");
				r.readEnum(info.action, "action");
				r.endGroup();
			}
			r.endGroup();
		}
	private:
		Map m_map;
	};
	typedef KeyModifierMap<Enum::ClickAction> ClickActionMap;
	typedef KeyModifierMap<Enum::WheelAction> WheelActionMap;

	bool pause_minimized{true}, pause_video_only{true};
	bool remember_stopped{true}, ask_record_found{true};
	bool enable_generate_playist{true};
	Enum::GeneratePlaylist generate_playlist{Enum::GeneratePlaylist::Folder};
	bool hide_cursor{true}, disable_screensaver{true};
	int hide_cursor_delay{3000};

	int blur_kern_c{1}, blur_kern_n{2}, blur_kern_d{1};
	int sharpen_kern_c{5}, sharpen_kern_n{-1}, sharpen_kern_d{0};
	int adjust_contrast_min_luma{16}, adjust_contrast_max_luma{235};

	bool sub_enable_autoload{true}, sub_enable_autoselect{true}, sub_enc_autodetection{true};
	Enum::SubtitleAutoload sub_autoload{Enum::SubtitleAutoload::Contain};
	Enum::SubtitleAutoselect sub_autoselect{Enum::SubtitleAutoselect::Matched};
	QString sub_enc{QLocale::system().language() == QLocale::Korean ? "CP949" : "UTF-8"}, sub_ext;
	int sub_enc_accuracy{70}, ms_per_char{500};
	OsdStyle sub_style{defaultSubtitleStyle()};		QStringList sub_priority;

	QLocale locale{QLocale::system()};
	bool enable_system_tray{true}, hide_rather_close{true};
	ClickActionMap double_click_map{defaultDoubleClick()}, middle_click_map{defaultMiddleClick()};
	WheelActionMap wheel_scroll_map{defaultWheelAction()};
	int seek_step1{5000}, seek_step2{30000}, seek_step3{60000}, speed_step{10};
	int brightness_step{1}, saturation_step{1}, contrast_step{1}, hue_step{1};
	int volume_step{2}, sync_delay_step{500}, amp_step{10}, sub_pos_step{1};

	bool enable_hwaccel{false};
	VideoFormat::Type hwaccel_format{VideoFormat::YV12};

	QString skin_name{"simple"};

	void save() const;
	void load();
private:
	static ClickActionMap defaultDoubleClick() {
		ClickActionMap map{false, Enum::ClickAction::Fullscreen};
		map[Enum::KeyModifier::None].enabled = true;
		return map;
	}
	static ClickActionMap defaultMiddleClick() {
		ClickActionMap map{false, Enum::ClickAction::Fullscreen};
		map[Enum::KeyModifier::None] = ClickActionInfo(true, Enum::ClickAction::Pause);
		return map;
	}
	static WheelActionMap defaultWheelAction() {
		WheelActionMap map(false, Enum::WheelAction::Volume);
		map[Enum::KeyModifier::None].enabled = true;
		map[Enum::KeyModifier::Ctrl] = WheelActionInfo(true, Enum::WheelAction::Amp);
		return map;
	}
	static OsdStyle defaultSubtitleStyle() {
		OsdStyle style;
		style.color = Qt::white;
		style.outline_color = Qt::black;
		style.shadow_color = Qt::black;
		style.shadow_color.setAlphaF(0.3);
		style.has_shadow = style.shadow_blur = style.has_outline = true;
		style.size = 0.035;
		style.outline_width = 0.0015;
		style.scale = OsdStyle::Scale::Width;
		style.shadow_offset = QPointF(0.003, 0.003);
		return style;
	}

	friend class PrefDialog;
	static Pref *obj;
//	Pref() {load();}
	friend int main(int, char**);
};


#endif // PREF_HPP
