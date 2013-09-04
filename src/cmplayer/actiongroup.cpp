#include "actiongroup.hpp"

ActionGroup::ActionGroup(QObject *parent)
: QActionGroup(parent) {}

QAction *ActionGroup::find(const QVariant &data) const {
	const auto actions = this->actions();
	for (auto action : actions) {
		if (action->data() == data)
			return action;
	}
	return nullptr;
}

void ActionGroup::setChecked(const QVariant &data, bool checked) {
	if (auto action = find(data))
		action->setChecked(checked);
}

void ActionGroup::trigger(double data) {
	for (auto action : actions()) {
		if (qFuzzyCompare(action->data().toDouble(), data)) {
			action->trigger();
			return;
		}
	}
}

void ActionGroup::trigger(const QVariant &data) {
	if (auto action = find(data))
		action->trigger();
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

