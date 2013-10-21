#ifndef ROOTMENU_HPP
#define ROOTMENU_HPP

#include "menu.hpp"

typedef QHash<QString, QList<QKeySequence> > Shortcuts;

class Pref;
template<typename E> struct ActionEnumInfo;
using ClickActionEnumInfo = ActionEnumInfo<ClickAction>;
using WheelActionEnumInfo = ActionEnumInfo<WheelAction>;

class RootMenu : public Menu {
	Q_OBJECT
private:
	struct WheelActionPair {
		WheelActionPair(QAction *up, QAction *down): up(up), down(down) {}
		WheelActionPair(): up(0), down(0) {}
		bool isNull() const {return !up || !down;}
		QAction *up, *down;
	};
	struct ArgAction { QString argument; QAction *action = nullptr; };
	typedef QMap<ClickAction, QAction*> ClickActionMap;
	typedef QMap<WheelAction, WheelActionPair> WheelActionMap;
public:
	enum Preset {Current, CMPlayer, Movist};
	RootMenu();
	RootMenu(const RootMenu &) = delete;
	~RootMenu() {obj = nullptr;}
	void update(const Pref &p);
	static inline RootMenu &instance() {return *obj;}
	QString longId(QAction *action) const {return m_ids.value(action);}
	QAction *action(const QString &longId) const {return m_actions.value(longId).action;}
	QAction *action(const QKeySequence &shortcut) const {return m_keymap.value(shortcut);}
	QAction *doubleClickAction(const ClickActionEnumInfo &info) const;
	QAction *middleClickAction(const ClickActionEnumInfo &info) const;
	QAction *wheelScrollAction(const WheelActionEnumInfo &info, bool up) const;
	inline void resetKeyMap() {m_keymap.clear(); fillKeyMap(this);}
	Shortcuts shortcuts() const;
	void setShortcuts(const Shortcuts &shortcuts);
	static bool execute(const QString &longId, const QString &argument = QString());
private:
	template<typename N>
	inline static void setActionAttr(QAction *act, const QVariant &data
			, const QString &text, N textValue, bool sign = true) {
		act->setData(data);
		act->setText(text.arg(toString(textValue, sign)));
	}

	inline static void setActionStep(QAction *plus, QAction *minus
			, const QString &text, int value, double textRate = -1.0) {
		if (textRate < 0) {
			plus->setText(text.arg(toString(value, true)));
			minus->setText(text.arg(toString(-value, true)));
		} else {
			plus->setText(text.arg(toString(value*textRate, true)));
			minus->setText(text.arg(toString(-value*textRate, true)));
		}
		plus->setData(value);
		minus->setData(-value);
	}

	inline static void setVideoPropStep(Menu &menu, const QString &key
			, VideoColor::Type prop, const QString &text, int step) {
		setActionAttr(menu[key + "+"], QList<QVariant>() << prop << step, text, step);
		setActionAttr(menu[key + "-"], QList<QVariant>() << prop << -step, text, -step);
	}
	void fillId(Menu *menu, const QString &parent);
	void fillKeyMap(Menu *menu);
	static RootMenu *obj;
	ClickActionMap m_click;
	WheelActionMap m_wheel;
	QHash<QAction*, QString> m_ids;
	QHash<QString, ArgAction> m_actions;
	QMap<QKeySequence, QAction*> m_keymap;
};

//class ShortcutMap {
//public:
//	typedef QList<QKeySequence> Keys;
//	ShortcutMap() {}
//	Keys operator[] (const QString &id) const {return m_map.value(id);}
//	Keys &operator[] (const QString &id) const {return m_map[id];}
//private:
////	QHash<QString, Keys> m_map;
//};

#endif // ROOTMENU_HPP
