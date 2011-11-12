#include "global.hpp"
#include <QtGui/QFileDialog>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QX11Info>
#include <QtCore/QRegExp>
#include <QtCore/QDebug>
#include <QtCore/QTime>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QDialog>

namespace Global {

QStringList getOpenFileNames(QWidget *p, const QString &t, const QString &dir, const QString &f) {
	return QFileDialog::getOpenFileNames(p, t, dir, f, 0, QFileDialog::DontUseNativeDialog);
}

QString getOpenFileName(QWidget *p, const QString &t, const QString &dir, const QString &f) {
	return QFileDialog::getOpenFileName(p, t, dir, f, 0, QFileDialog::DontUseNativeDialog);
}

QString getSaveFileName(QWidget *p, const QString &t, const QString &dir, const QString &f) {
	return QFileDialog::getSaveFileName(p, t, dir, f, 0, QFileDialog::DontUseNativeDialog);
}

QDialogButtonBox *makeButtonBox(QDialog *dlg) {
	QDialogButtonBox *dbb = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dlg);
	dlg->connect(dbb, SIGNAL(accepted()), SLOT(accept()));
	dlg->connect(dbb, SIGNAL(rejected()), SLOT(reject()));
	return dbb;
}

}
