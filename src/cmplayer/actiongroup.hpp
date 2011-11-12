#ifndef ACTIONGROUP_HPP
#define ACTIONGROUP_HPP

#include <QtGui/QActionGroup>
#include <QtCore/QUrl>

class Mrl;

class ActionGroup : public QActionGroup {
	Q_OBJECT
public:
	ActionGroup(QObject *parent = 0);
	void setChecked(const QVariant &data, bool checked);
	void trigger(const QVariant &data);
	QVariant data() const;
	void clear();
signals:
	void triggered(const QVariant &data);
	void triggered(int data);
	void triggered(double data);
	void triggered(const QString &data);
	void triggered(const Mrl &data);
private slots:
	void emitData(QAction *action);
};

#endif // ACTIONGROUP_HPP
