#ifndef ACTIONGROUP_HPP
#define ACTIONGROUP_HPP

#include "stdafx.hpp"

class Mrl;

class ActionGroup : public QActionGroup {
	Q_OBJECT
public:
	ActionGroup(QObject *parent = 0);
	void setChecked(const QVariant &data, bool checked);
	void trigger(const QVariant &data);
	QVariant data() const;
	void clear();
//signals:
//	void varTriggered(const QVariant &data);
//	void intTriggered(int data);
//	void doubleTriggered(double data);
//	void stringTriggered(const QString &data);
//	void mrlTriggered(const Mrl &data);
private slots:
//	void emitData(QAction *action);
};

#endif // ACTIONGROUP_HPP
