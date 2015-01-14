#include "actiongroup.hpp"

ActionGroup::ActionGroup(QObject *parent)
    : QActionGroup(parent) { }

auto ActionGroup::firstCheckedAction() const -> QAction*
{
    for (auto action : actions()) {
        if (action->isChecked())
            return action;
    }
    return nullptr;
}

auto ActionGroup::lastCheckedAction() const -> QAction*
{
    const auto actions = this->actions();
    for (int i = actions.size()-1; i >= 0; --i) {
        if (actions[i]->isChecked())
            return actions[i];
    }
    return nullptr;
}

auto ActionGroup::find(const QVariant &data) const -> QAction*
{
    for (auto action : actions()) {
        if (action->data() == data)
            return action;
    }
    return nullptr;
}

auto ActionGroup::setChecked(const QVariant &data, bool checked) -> void
{
    if (auto action = find(data))
        action->setChecked(checked);
}

auto ActionGroup::trigger(double data) -> void
{
    for (auto action : actions()) {
        if (qFuzzyCompare(action->data().toDouble(), data)) {
            action->trigger();
            return;
        }
    }
}

auto ActionGroup::trigger(const QVariant &data) -> void
{
    if (auto action = find(data))
        action->trigger();
}

auto ActionGroup::data() const -> QVariant
{
    const auto action = checkedAction();
    return action ? action->data() : QVariant();
}

auto ActionGroup::clear() -> void
{
    const auto actions = this->actions();
    for (auto action : actions) {
        removeAction(action);
        delete action;
    }
}

