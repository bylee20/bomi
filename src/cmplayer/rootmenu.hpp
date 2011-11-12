#ifndef ROOTMENU_HPP
#define ROOTMENU_HPP

#include "menu.hpp"

class RootMenu : public Menu {
	struct WheelActionPair {
		WheelActionPair(QAction *up, QAction *down): up(up), down(down) {}
		WheelActionPair(): up(0), down(0) {}
		bool isNull() const {return !up || !down;}
		QAction *up, *down;
	};
	typedef QMap<Enum::ClickAction, QAction*> ClickActionMap;
	typedef QMap<Enum::WheelAction, WheelActionPair> WheelActionMap;
public:
	RootMenu();
	~RootMenu();
	void save();
	void load();
	void update();
	inline QAction *clickAction(const Enum::ClickAction &a) const {return m_click[a];}
	inline QAction *wheelAction(const Enum::WheelAction &a, bool up) const {
		return up ? m_wheel[a].up : m_wheel[a].down;
	}
	static inline RootMenu &get() {Q_ASSERT(obj != 0); return *obj;}
	QAction *action(const QString &id) const;
	QAction *doubleClickAction(Qt::KeyboardModifiers mod) const;
	QAction *middleClickAction(Qt::KeyboardModifiers mod) const;
	QAction *wheelScrollAction(Qt::KeyboardModifiers mod, bool up) const;
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
			, ColorProperty::Value prop, const QString &text, int step) {
		setActionAttr(menu[key + "+"], QList<QVariant>() << prop << step, text, step);
		setActionAttr(menu[key + "-"], QList<QVariant>() << prop << -step, text, -step);
	}
	static RootMenu *obj;
	ClickActionMap m_click;
	WheelActionMap m_wheel;
};

#endif // ROOTMENU_HPP
