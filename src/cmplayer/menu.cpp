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

QMenu *Menu::copied(QWidget *parent) {
	QMenu *menu = new QMenu(parent);
	menu->setTitle(title());
	menu->setEnabled(isEnabled());
	connect(menu, SIGNAL(triggered(QAction*)), this, SIGNAL(triggered(QAction*)));
	for (QAction *action : actions()) {
		auto sub = static_cast<Menu*>(action->menu());
		if (sub)
			menu->addMenu(sub->copied(menu));
		else
			menu->QMenu::addAction(action);
	}
	m_copies << menu;
	return menu;
}
