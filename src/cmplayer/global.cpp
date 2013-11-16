#include "global.hpp"

namespace Global {

QDialogButtonBox *makeButtonBox(QDialog *dlg) {
	QDialogButtonBox *dbb = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dlg);
	dlg->connect(dbb, SIGNAL(accepted()), SLOT(accept()));
	dlg->connect(dbb, SIGNAL(rejected()), SLOT(reject()));
	return dbb;
}

}
