#include "menu.hpp"
#include "rootmenu.hpp"
#include "record.hpp"

template<class T>
auto toStringList(const QList<T> &list) -> QStringList
{
    QStringList ret;
    ret.reserve(list.size());
    for (int i=0; i<list.size(); ++i)
        ret.push_back(list[i].toString());
    return ret;
}

template<class T>
auto fromStringList(const QStringList &list) -> QList<T>
{
    QList<T> ret;
    ret.reserve(list.size());
    for (int i=0; i<list.size(); ++i)
        ret.push_back(T::fromString(list[i]));
    return ret;
}

Menu::Menu(const QString &id, QWidget *parent)
    : QMenu(parent)
    , m_id(id)
{
    addGroup("");
}

auto Menu::copied(QWidget *parent) -> QMenu*
{
    QMenu *menu = new QMenu(parent);
    menu->setTitle(title());
    menu->setEnabled(isEnabled());
    connect(menu, &QMenu::triggered, this, &Menu::triggered);
    for (auto action : actions()) {
        auto sub = static_cast<Menu*>(action->menu());
        if (sub)
            menu->addMenu(sub->copied(menu));
        else
            menu->QMenu::addAction(action);
    }
    m_copies.push_back(menu);
    return menu;
}
