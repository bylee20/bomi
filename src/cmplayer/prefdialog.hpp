#ifndef PREFDIALOG_HPP
#define PREFDIALOG_HPP

#include "stdafx.hpp"

class QTreeWidgetItem;		class QAbstractButton;
class Pref;

class PrefDialog : public QDialog {
	Q_OBJECT
public:
	PrefDialog(QWidget *parent = 0);
	~PrefDialog();
	void set(const Pref &pref);
	void get(Pref &p);
signals:
	void applyRequested();
	void resetRequested();
private:
	void setShortcuts(const QHash<QString, QList<QKeySequence> > &shortcuts);
	void changeEvent(QEvent *event);
	void showEvent(QShowEvent *event);
	QString toString(const QLocale &locale);
	void retranslate();
	class MenuTreeItem;
	class Delegate;
	struct Data;
	Data *d;
};

#endif // PREFDIALOG_HPP
