#include "actiongroup.hpp"

ActionGroup::ActionGroup(QObject *parent)
: QActionGroup(parent) {
//	connect(this, SIGNAL(triggered(QAction*)), this, SLOT(emitData(QAction*)));
}

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

//void ActionGroup::emitData(QAction *action) {
//	const QVariant data = action->data();
//	emit triggered(data);
//	if (data.type() == QVariant::Int)
//		emit triggered(data.toInt());
//	else if (data.type() == QVariant::Double)
//		emit triggered(data.toDouble());
//	else if (data.type() == QVariant::String)
//		emit triggered(data.toString());
//	else if (data.type() == QVariant::Url)
//		emit triggered(data.toUrl().toString());
//}
