#include "actiongroup.hpp"

ActionGroup::ActionGroup(QObject *parent)
: QActionGroup(parent) {}

void ActionGroup::setChecked(const QVariant &data, bool checked) {
	const auto actions = this->actions();
	for (auto action : actions) {
		if (action->data() == data) {
			action->setChecked(checked);
			return;
		}
	}
}

void ActionGroup::trigger(const QVariant &data) {
	const auto actions = this->actions();
	for (auto action : actions) {
		if (action->data() == data) {
			action->trigger();
			return;
		}
	}
}

QVariant ActionGroup::data() const {
	const auto action = checkedAction();
	return action ? action->data() : QVariant();
}

void ActionGroup::clear() {
	const auto actions = this->actions();
	for (auto action : actions) {
		removeAction(action);
		delete action;
	}
}
