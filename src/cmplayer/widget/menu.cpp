#include "menu.hpp"

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
