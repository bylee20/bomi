#include "menu.hpp"
#include "rootmenu.hpp"
#include "record.hpp"

template<typename T>
QStringList toStringList(const QList<T> &list) {
	QStringList ret;
	ret.reserve(list.size());
	for (int i=0; i<list.size(); ++i)
		ret.push_back(list[i].toString());
	return ret;
}

template<typename T>
QList<T> fromStringList(const QStringList &list) {
	QList<T> ret;
	ret.reserve(list.size());
	for (int i=0; i<list.size(); ++i)
		ret.push_back(T::fromString(list[i]));
	return ret;
}

Menu::Menu(const QString &id, QWidget *parent)
: QMenu(parent), m_id(id) {
	addGroup("");
}

void Menu::save(Record &r) const {
	r.beginGroup(m_id);
	for (ActionHash::const_iterator it = m_a.begin(); it != m_a.end(); ++it) {
		r.beginGroup(it.key());
		r.write(toStringList((*it)->shortcuts()), "shortcut");
		r.endGroup();
	}
	for (MenuHash::const_iterator it = m_m.begin(); it != m_m.end(); ++it)
		(*it)->save(r);
	r.endGroup();
}

void Menu::load(Record &r) {
	r.beginGroup(m_id);
	for (ActionHash::const_iterator it = m_a.begin(); it != m_a.end(); ++it) {
		r.beginGroup(it.key());
		QStringList keys = {_L("none")};
		r.read(keys, "shortcut");
		if (keys.size() != 1 || keys[0] != _L("none"))
			(*it)->setShortcuts(fromStringList<QKeySequence>(keys));
		r.endGroup();
	}
	for (MenuHash::const_iterator it = m_m.begin(); it != m_m.end(); ++it)
		(*it)->load(r);
	r.endGroup();
}
