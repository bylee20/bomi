#include "actiongroup.hpp"

ActionGroup::ActionGroup(QObject *parent)
    : QActionGroup(parent) { }

QAction *ActionGroup::firstCheckedAction() const
{
    for (auto action : actions()) {
        if (action->isChecked())
            return action;
    }
    return nullptr;
}

QAction *ActionGroup::lastCheckedAction() const
{
    const auto actions = this->actions();
    for (int i = actions.size()-1; i >= 0; --i) {
        if (actions[i]->isChecked())
            return actions[i];
    }
    return nullptr;
}

QAction *ActionGroup::find(const QVariant &data) const
{
    for (auto action : actions()) {
        if (action->data() == data)
            return action;
    }
    return nullptr;
}

void ActionGroup::setChecked(const QVariant &data, bool checked)
{
    if (auto action = find(data))
        action->setChecked(checked);
}

void ActionGroup::trigger(double data)
{
    for (auto action : actions()) {
        if (qFuzzyCompare(action->data().toDouble(), data)) {
            action->trigger();
            return;
        }
    }
}

void ActionGroup::trigger(const QVariant &data)
{
    if (auto action = find(data))
        action->trigger();
}

QVariant ActionGroup::data() const
{
    const auto action = checkedAction();
    return action ? action->data() : QVariant();
}

void ActionGroup::clear()
{
    const auto actions = this->actions();
    for (auto action : actions) {
        removeAction(action);
        delete action;
    }
}

