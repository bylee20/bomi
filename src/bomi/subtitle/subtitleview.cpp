#include "subtitleview.hpp"
#include "subtitlemodel.hpp"
#include "subtitle.hpp"
#include "misc/objectstorage.hpp"
#include <QSplitter>
#include <QScrollArea>

class SubtitleView::CompView : public QWidget {
public:
    CompView(QWidget *parent = nullptr);
    auto setModel(SubCompModel *model) -> void;
    auto view() const -> SubCompView* {return m_view;}
private:
    SubCompView *m_view;
    QLabel *m_name;
};

SubtitleView::CompView::CompView(QWidget *parent)
    : QWidget(parent)
{
    m_view = new SubCompView(this);
    m_name = new QLabel(this);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->addWidget(m_name);
    vbox->addWidget(m_view);
}

auto SubtitleView::CompView::setModel(SubCompModel *model) -> void
{
    if (model)
        m_name->setText(model->name());
    m_view->setModel(model);
}

/******************************************************************************/

struct SubtitleView::Data {
    QVector<SubCompModel*> models;
    QList<CompView*> comp;
    QSplitter *splitter;
    QCheckBox *timeVisible, *autoScroll;
    bool needToUpdate = false;
    ObjectStorage storage;
};

SubtitleView::SubtitleView(QWidget *parent)
    : QDialog(parent)
    , d(new Data)
{
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

    connect(d->timeVisible, &QCheckBox::toggled,
            this, &SubtitleView::setTimeVisible);
    connect(d->autoScroll, &QCheckBox::toggled,
            this, &SubtitleView::setAutoScrollEnabled);

    _SetWindowTitle(this, tr("Subtitle Viewer"));

    d->storage.setObject(this, u"subtitle-viewer"_q, true);
    d->storage.add("time_visible", d->timeVisible, "checked");
    d->storage.add("auto_scroll", d->autoScroll, "checked");
    d->storage.restore();
}

SubtitleView::~SubtitleView()
{
    delete d;
}

auto SubtitleView::updateModels() -> void
{
    if (d->models.isEmpty()) {
        d->splitter->setVisible(false);
        for (int i=0; i<d->comp.size(); ++i)
            d->comp[i]->setModel(nullptr);
    } else  {
        while (d->comp.size() > d->models.size())
            delete d->comp.takeLast();
        while (d->comp.size() < d->models.size()) {
            CompView *comp = new CompView(d->splitter);
            d->splitter->addWidget(comp);
            d->comp.push_back(comp);
        }
        for (int i=0; i<d->comp.size(); ++i) {
            d->comp[i]->setModel(d->models[i]);
            d->comp[i]->view()->setAutoScrollEnabled(d->autoScroll->isChecked());
            d->comp[i]->view()->setTimeVisible(d->timeVisible->isChecked());
        }
        d->models.clear();
    }
    d->needToUpdate = false;
}

auto SubtitleView::setModels(const QVector<SubCompModel*> &models) -> void
{
    d->models = models;
    if (isVisible())
        updateModels();
    else
        d->needToUpdate = true;
}

auto SubtitleView::setTimeVisible(bool visible) -> void
{
    for (int i=0; i<d->comp.size(); ++i)
        d->comp[i]->view()->setTimeVisible(visible);
}

auto SubtitleView::setAutoScrollEnabled(bool enabled) -> void
{
    for (int i=0; i<d->comp.size(); ++i)
        d->comp[i]->view()->setAutoScrollEnabled(enabled);
}

auto SubtitleView::showEvent(QShowEvent *event) -> void
{
    QDialog::showEvent(event);
    if (d->needToUpdate)
        updateModels();
}
