#ifndef MENU_HPP
#define MENU_HPP

#include "global.hpp"
#include "actiongroup.hpp"
#include "enums.hpp"
#include "colorproperty.hpp"
#include <QtGui/QMenu>
#include <QtCore/QHash>

typedef QLatin1String _LS;
typedef QLatin1Char _LC;

class Record;

class Menu : public QMenu {
	Q_OBJECT
public:
	struct WheelActionPair {
		WheelActionPair(QAction *up, QAction *down): up(up), down(down) {}
		WheelActionPair(): up(0), down(0) {}
		bool isNull() const {return !up || !down;}
		QAction *up, *down;
	};
	typedef QMap<Enum::ClickAction, QAction*> ClickActionMap;
	typedef QMap<Enum::WheelAction, WheelActionPair> WheelActionMap;
	typedef QHash<QString, Menu*> MenuHash;
	typedef QHash<QString, QAction*> ActionHash;
	typedef QHash<QString, ActionGroup*> GroupHash;

	inline Menu &operator() (const QLatin1String &key) const {return *m(key);}
	inline QAction *operator[] (const QLatin1String &key) const {return a(key);}
	inline ActionGroup *g(const QLatin1String &key) const {return m_g[key];}
	inline QAction *a(const QLatin1String &key) const {return m_a[key];}
	inline Menu *m(const QLatin1String &key) const {return m_m[key];}

	inline Menu &operator() (const QString &key) const {return *m(key);}
	inline QAction *operator[] (const QString &key) const {return a(key);}
	inline ActionGroup *g(const QString &key = "") const {return m_g[key];}
	inline QAction *a(const QString &key) const {return m_a[key];}
	inline Menu *m(const QString &key) const {return m_m[key];}

	inline Menu *addMenu(const QString &key) {
		Menu *m = m_m[key] = new Menu(key, this); QMenu::addMenu(m); return m;
	}
	inline QAction *addActionToGroup(const QString &key, bool ch = false, const QString &g = "") {
		return addGroup(g)->addAction(addAction(key, ch));
	}
	inline QAction *addAction(const QString &key, bool ch = false) {
		QAction *a = m_a[key] = QMenu::addAction(key); a->setCheckable(ch); return a;
	}
	inline QAction *addActionToGroupWithoutKey(const QString &name, bool ch = false, const QString &g = "") {
		QAction *a = QMenu::addAction(name); a->setCheckable(ch); return addGroup(g)->addAction(a);
	}
	inline ActionGroup *addGroup(const QString &name) {
		ActionGroup *g = m_g.value(name, 0); return g ? g : (m_g[name] = new ActionGroup(this));
	}
	inline QString id(QAction *action) const {return m_a.key(action, QString());}
	inline QString id(Menu *menu) const {return m_m.key(menu, QString());}
	inline QString id() const {return m_id;}
protected:
	Menu(const QString &id, Menu *parent);
	void save(Record &set) const;
	void load(Record &set);
private:
	GroupHash m_g;
	ActionHash m_a;
	MenuHash m_m;
	const QString m_id;
};

#endif // MENU_HPP
