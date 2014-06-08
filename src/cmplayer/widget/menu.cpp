#include "menu.hpp"
#include "misc/stepactionpair.hpp"

Menu::Menu(const QString &id, QWidget *parent)
    : QMenu(parent)
    , m_id(id)
{
    addGroup(QString());
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

auto Menu::addStepActionPair(const QString &inc, const QString &dec,
                             const QString &pair,
                             const QString &group) -> StepActionPair*
{
    auto p = new StepActionPair(this);
    addActionToGroup(p->increase(), inc, group);
    addActionToGroup(p->decrease(), dec, group);
    return m_s[pair] = p;
}
