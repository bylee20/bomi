#include "simplelistwidget.hpp"
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QDialog>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>

struct SimpleListWidget::Data {
	QListWidget *list;
	QPushButton *add, *erase, *up, *down;
	bool addable;
};

SimpleListWidget::SimpleListWidget(QWidget *parent)
: QWidget(parent), d(new Data) {
	d->list = new QListWidget(this);
	d->add = new QPushButton(tr("Add"), this);
	d->erase = new QPushButton(tr("Erase"), this);
	d->up = new QPushButton(tr("Move Up"), this);
	d->down = new QPushButton(tr("Move Down"), this);

	QVBoxLayout *vbox = new QVBoxLayout(this);
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget(d->add);
	hbox->addWidget(d->erase);
	hbox->addWidget(d->up);
	hbox->addWidget(d->down);
	vbox->addLayout(hbox);
	vbox->addWidget(d->list);
	vbox->setMargin(0);
	hbox->setMargin(0);

	connect(d->list, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*))
			, this, SLOT(slotCurrentItemChanged(QListWidgetItem*)));
	connect(d->add, SIGNAL(clicked()), this, SLOT(slotAdd()));
	connect(d->erase, SIGNAL(clicked()), this, SLOT(slotErase()));
	connect(d->up, SIGNAL(clicked()), this, SLOT(slotUp()));
	connect(d->down, SIGNAL(clicked()), this, SLOT(slotDown()));

	slotCurrentItemChanged(0);
	setAddingAndErasingEnabled(false);
}

SimpleListWidget::~SimpleListWidget() {
	delete d;
}

void SimpleListWidget::slotCurrentItemChanged(QListWidgetItem *item) {
	d->erase->setEnabled(item && d->addable);
	d->up->setEnabled(item && d->list->row(item) > 0);
	d->down->setEnabled(item && d->list->row(item) < d->list->count() - 1);
}

void SimpleListWidget::slotErase() {
	delete d->list->currentItem();
}

void SimpleListWidget::slotUp() {
	const int row = d->list->currentRow();
	if (row > 0) {
		QListWidgetItem *item = d->list->takeItem(row);
		d->list->insertItem(row-1, item);
		d->list->setCurrentItem(item);
	}
}

void SimpleListWidget::slotDown() {
	const int row = d->list->currentRow();
	if (row < d->list->count() - 1) {
		QListWidgetItem *item = d->list->takeItem(row);
		d->list->insertItem(row+1, item);
		d->list->setCurrentItem(item);
	}
}

void SimpleListWidget::slotAdd() {
	QDialog dlg(this);
	dlg.setWindowTitle(tr("Add"));
	QLineEdit *edit = new QLineEdit(&dlg);
	QPushButton *ok = new QPushButton(tr("&Ok"), &dlg);
	QPushButton *cancel = new QPushButton(tr("&Cancel"), &dlg);
	QHBoxLayout *hbox = new QHBoxLayout(&dlg);
	hbox->addWidget(edit);
	hbox->addWidget(ok);
	hbox->addWidget(cancel);
	connect(ok, SIGNAL(clicked()), &dlg, SLOT(accept()));
	connect(cancel, SIGNAL(clicked()), &dlg, SLOT(reject()));
	if (dlg.exec())
		d->list->addItem(edit->text());
}

void SimpleListWidget::setValues(const QStringList &values) {
	d->list->clear();
	d->list->addItems(values);
}

QStringList SimpleListWidget::values() const {
	QStringList values;
	for (int i=0; i<d->list->count(); ++i)
		values << d->list->item(i)->text();
	return values;
}

void SimpleListWidget::setAddingAndErasingEnabled(bool enabled) {
	d->addable = enabled;
	d->add->setEnabled(d->addable);
	d->erase->setEnabled(d->addable);
	d->add->setVisible(d->addable);
	d->erase->setVisible(d->addable);
}
