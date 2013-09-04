#ifndef ACTIONGROUP_HPP
#define ACTIONGROUP_HPP

#include "stdafx.hpp"

class Mrl;

class ActionGroup : public QActionGroup {
	Q_OBJECT
public:
	ActionGroup(QObject *parent = 0);
	void setChecked(const QVariant &data, bool checked);
	void trigger(double data);
	void trigger(const QVariant &data);
	QVariant data() const;
	void clear();
	QAction *find(const QVariant &data) const;
};

#endif // ACTIONGROUP_HPP
