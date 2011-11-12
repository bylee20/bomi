#ifndef PREF_DIALOG_HPP
#define PREF_DIALOG_HPP

#include "pref.hpp"
#include <QtGui/QDialog>
#include <QtGui/QMainWindow>

class QAbstractButton;

#ifdef Q_WS_MAC
class Pref::Dialog : public QMainWindow {
#else
class Pref::Dialog : public QDialog {
#endif
	Q_OBJECT
public:
	Dialog(QWidget *parent = 0);
signals:
	void needToApplyPref();
private slots:
	void setCurrentPage(QAction *action);
	void buttonClicked(QAbstractButton *button);
private:
	void showEvent(QShowEvent *event);
	struct Data;
	Data *d;
};

#endif // PREF_DIALOG_HPP
