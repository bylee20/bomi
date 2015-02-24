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

static inline auto cmpFixed(double lhs, double rhs, double times = 1e10) -> bool
{
    return std::llround(lhs * times) == std::llround(rhs * times);
}

auto ActionGroup::find(const QVariant &data) const -> QAction*
{
    switch (data.userType()) {
    case QMetaType::Double:
    case QMetaType::Float:
        for (auto action : actions()) {
            if (cmpFixed(action->data().toDouble(), data.toDouble()))
                return action;
        }
        return nullptr;
    default:
        for (auto action : actions()) {
            if (action->data() == data)
                return action;
        }
        return nullptr;
    }
}

auto ActionGroup::setChecked(const QVariant &data, bool checked) -> QAction*
{
    if (auto action = find(data)) {
        action->setChecked(checked);
        return action;
    }
    setAllChecked(false);
    return nullptr;
}

auto ActionGroup::setAllChecked(bool checked) -> void
{
    for (auto a : actions())
        a->setChecked(checked);
}

auto ActionGroup::trigger(const QVariant &data) -> QAction*
{
    if (auto action = find(data)) {
        action->trigger();
        return action;
    }
    return nullptr;
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

