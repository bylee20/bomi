#include "actiongroup.hpp"

ActionGroup::ActionGroup(QObject *parent)
: QActionGroup(parent) {
	connect(this, SIGNAL(triggered(QAction*)), this, SLOT(emitData(QAction*)));
}

void ActionGroup::setChecked(const QVariant &data, bool checked) {
	const QList<QAction*> acts = actions();
	for (QAction *act : acts) {
		if (act->data() == data) {
			act->setChecked(checked);
			return;
		}
	}
}

void ActionGroup::trigger(const QVariant &data) {
	const QList<QAction*> acts = actions();
	for (QAction *act : acts) {
		if (act->data() == data) {
			act->trigger();
			return;
		}
	}
}

QVariant ActionGroup::data() const {
	const QAction *act = checkedAction();
	return act ? act->data() : QVariant();
}

void ActionGroup::clear() {
	const QList<QAction*> acts = actions();
	for (QAction *act : acts) {
		removeAction(act);
		delete act;
	}
}

void ActionGroup::emitData(QAction *action) {
	const QVariant data = action->data();
	emit triggered(data);
	if (data.type() == QVariant::Int)
		emit triggered(data.toInt());
	else if (data.type() == QVariant::Double)
		emit triggered(data.toDouble());
	else if (data.type() == QVariant::String)
		emit triggered(data.toString());
	else if (data.type() == QVariant::Url)
		emit triggered(data.toUrl().toString());
}
