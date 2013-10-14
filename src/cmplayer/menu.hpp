#ifndef MENU_HPP
#define MENU_HPP

#include "stdafx.hpp"
#include "global.hpp"
#include "actiongroup.hpp"
#include "enums.hpp"
#include "colorproperty.hpp"

class Record;

class Menu : public QMenu {
	Q_OBJECT
public:
	static constexpr int IdRole = Qt::UserRole + 1;
	struct WheelActionPair {
		WheelActionPair(QAction *up, QAction *down): up(up), down(down) {}
		WheelActionPair(): up(0), down(0) {}
		bool isNull() const {return !up || !down;}
		QAction *up, *down;
	};
	typedef QMap<ClickAction, QAction*> ClickActionMap;
	typedef QMap<WheelAction, WheelActionPair> WheelActionMap;
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
		Menu *m = m_m[key] = new Menu(key, this); QMenu::addMenu(m); m_ids[key] = m->menuAction(); return m;
	}
	inline QAction *addActionToGroup(const QString &key, bool ch = false, const QString &g = "") {
		return m_ids[key] = addGroup(g)->addAction(addAction(key, ch));
	}
	inline QAction *addAction(const QString &key, bool ch = false) {
		QAction *a = m_a[key] = QMenu::addAction(key); a->setCheckable(ch); m_ids[key] = a; return a;
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
	QMenu *copied(QWidget *parent = nullptr);
	QList<QMenu*> copies() const {return m_copies;}
	void setEnabledSync(bool enabled) {setEnabled(enabled); for (QMenu *m : m_copies) m->setEnabled(enabled);}
	void syncTitle() {for (QMenu *m : m_copies) m->setTitle(title()); for (Menu *m : m_m) m->syncTitle();}
	void syncActions() {for (QMenu *m : m_copies) {m->addActions(actions());}}
	inline QHash<QString, QAction*> ids() const {return m_ids;}
protected:
	Menu(const QString &id, QWidget *parent);
//	void save(Record &set) const;
//	void load(Record &set);
private:
	GroupHash m_g;
	ActionHash m_a;
	MenuHash m_m;
	QHash<QString, QAction*> m_ids;
	QList<QMenu*> m_copies;
	const QString m_id;
	friend class MenuBar;
};

#endif // MENU_HPP
