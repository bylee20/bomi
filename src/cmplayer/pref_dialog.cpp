#include "pref_dialog.hpp"
#include "pref_widget.hpp"
#include <QtGui/QToolBar>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QActionGroup>
#include <QtGui/QListWidget>

struct Pref::Dialog::Data {
	Widget *widget;
	QDialogButtonBox *dbb;
};

Pref::Dialog::Dialog(QWidget *parent)
#ifdef Q_WS_MAC
: QMainWindow(parent, Qt::Dialog)
#else
: QDialog(parent)
#endif
, d(new Data) {
	d->widget = new Widget(this);
	d->dbb = new QDialogButtonBox(QDialogButtonBox::Apply |
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
#ifdef Q_WS_MAC
	QToolBar *tb = new QToolBar(this);
	tb->setIconSize(QSize(32, 32));
	QActionGroup *g = new QActionGroup(this);
	for (int i=0; i<d->widget->pageCount(); ++i) {
		QAction *act = tb->addAction(d->widget->pageIcon(i), d->widget->pageName(i));
		act->setData(i);
		act->setCheckable(true);
		g->addAction(act);
	}
	g->actions()[d->widget->currentPage()]->setChecked(true);
	tb->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
	addToolBar(tb);
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(d->widget);
	vbox->setMargin(0);
	vbox->addWidget(d->dbb);
	QWidget *c = new QWidget(this);
	c->setLayout(vbox);
	setCentralWidget(c);
	setUnifiedTitleAndToolBarOnMac(true);
	connect(g, SIGNAL(triggered(QAction*)), this, SLOT(setCurrentPage(QAction*)));
#else
	QHBoxLayout *hbox = new QHBoxLayout;
	QListWidget *list = new QListWidget;
	QFont font = this->font();
	list->setIconSize(QSize(48, 48));
	list->setViewMode(QListView::IconMode);
	list->setMovement(QListView::Static);
	list->setFixedWidth(90);
	list->setGridSize(QSize(80, 80));
//	font.setPixelSize(40);
	for (int i=0; i<d->widget->pageCount(); ++i) {
		QListWidgetItem *item = new QListWidgetItem(d->widget->pageIcon(i), d->widget->pageName(i));
//		item->setSizeHint(QSize(100, 100));
		item->setFont(font);
		list->addItem(item);
	}
	connect(list, SIGNAL(currentRowChanged(int)), d->widget, SLOT(setCurrentPage(int)));
	list->setCurrentRow(d->widget->currentPage());
	hbox->addWidget(list);
	hbox->addWidget(d->widget);
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addLayout(hbox);
	vbox->addWidget(d->dbb);
	setLayout(vbox);
#endif
	connect(d->dbb, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
}

void Pref::Dialog::showEvent(QShowEvent *event) {
#ifdef Q_WS_MAC
	QMainWindow::showEvent(event);
#else
	QDialog::showEvent(event);
#endif
	d->widget->fill();
	setWindowTitle(QLatin1String("CMPlayer - ") + tr("Preferences"));
}

void Pref::Dialog::setCurrentPage(QAction *action) {
	if (!action || !action->isChecked())
		return;
	d->widget->setCurrentPage(action->data().toInt());
}

void Pref::Dialog::buttonClicked(QAbstractButton *button) {
	const QDialogButtonBox::ButtonRole role = d->dbb->buttonRole(button);
	if (role == QDialogButtonBox::AcceptRole) {
		d->widget->apply();
		hide();
		emit needToApplyPref();
	} else if (role == QDialogButtonBox::ApplyRole) {
		d->widget->apply();
		emit needToApplyPref();
	} else if (role == QDialogButtonBox::RejectRole)
		hide();
}

