#include "subtitleview.hpp"
#include "subtitlemodel.hpp"
#include <QtGui/QTreeView>
#include <QtGui/QTableView>
#include "playengine.hpp"
#include "subtitle.hpp"
#include <QtGui/QVBoxLayout>
#include <QtGui/QTreeWidget>
#include <QtGui/QScrollArea>
#include <QtGui/QLabel>
#include <QtCore/QDebug>
#include <QtCore/QTime>
#include <QtGui/QSplitter>
#include <QtGui/QCheckBox>

class SubtitleView::CompView : public QWidget {
public:
	CompView(QWidget *parent = 0): QWidget(parent) {
		m_view = new SubtitleComponentView(this);
		m_name = new QLabel(this);

		QVBoxLayout *vbox = new QVBoxLayout(this);
		vbox->addWidget(m_name);
		vbox->addWidget(m_view);
	}
	void setModel(SubtitleComponentModel *model) {
		if (model)
			m_name->setText(model->name());
		m_view->setModel(model);
	}
	SubtitleComponentView *view() const {return m_view;}
private:
	SubtitleComponentView *m_view;
	QLabel *m_name;
};

struct SubtitleView::Data {
	QList<CompView*> comp;
	QSplitter *splitter;
	QCheckBox *timeVisible, *autoScroll;
};

SubtitleView::SubtitleView(QWidget *parent)
: ToggleDialog(parent), d(new Data) {
	QScrollArea *area = new QScrollArea(this);
	d->splitter = new QSplitter(Qt::Horizontal, area);
	d->timeVisible = new QCheckBox(tr("Show start/end time"), this);
	d->autoScroll = new QCheckBox(tr("Scroll to current time"), this);

	area->setWidget(d->splitter);
	area->setWidgetResizable(true);
	d->autoScroll->setChecked(true);
	d->timeVisible->setChecked(false);

	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->addWidget(area);
	vbox->addWidget(d->timeVisible);
	vbox->addWidget(d->autoScroll);

	setAutoScrollEnabled(d->autoScroll->isChecked());
	setTimeVisible(d->timeVisible->isChecked());

	connect(d->timeVisible, SIGNAL(toggled(bool)), this, SLOT(setTimeVisible(bool)));
	connect(d->autoScroll, SIGNAL(toggled(bool)), this, SLOT(setAutoScrollEnabled(bool)));

	setWindowTitle(tr("Subtitle View"));
}

SubtitleView::~SubtitleView() {
	delete d;
}

void SubtitleView::setModel(const QVector<SubtitleComponentModel*> &model) {
	while (d->comp.size() > model.size())
		delete d->comp.takeLast();
	while (d->comp.size() < model.size()) {
		CompView *comp = new CompView(d->splitter);
		d->splitter->addWidget(comp);
		d->comp.push_back(comp);
	}
	for (int i=0; i<d->comp.size(); ++i) {
		d->comp[i]->setModel(model[i]);
		d->comp[i]->view()->setAutoScrollEnabled(d->autoScroll->isChecked());
		d->comp[i]->view()->setTimeVisible(d->timeVisible->isChecked());
	}
}

void SubtitleView::setTimeVisible(bool visible) {
	for (int i=0; i<d->comp.size(); ++i)
		d->comp[i]->view()->setTimeVisible(visible);
}

void SubtitleView::setAutoScrollEnabled(bool enabled) {
	for (int i=0; i<d->comp.size(); ++i)
		d->comp[i]->view()->setAutoScrollEnabled(enabled);
}

