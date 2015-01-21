#include "menu.hpp"
#include "misc/stepactionpair.hpp"

Menu::Menu(const QString &id, QWidget *parent)
    : QMenu(parent)
    , m_key(id)
{
    addGroup(QString());
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
