#include "subtitleviewer.hpp"
#include "subtitlemodel.hpp"
#include "subtitle.hpp"
#include "misc/objectstorage.hpp"
#include <QSplitter>
#include <QScrollArea>

class SubtitleViewer::CompView : public QWidget {
public:
    CompView(QWidget *parent = nullptr);
    auto setModel(SubCompModel *model) -> void;
    auto view() const -> SubCompView* {return m_view;}
private:
    SubCompView *m_view;
    QLabel *m_name;
};

SubtitleViewer::CompView::CompView(QWidget *parent)
    : QWidget(parent)
{
    m_view = new SubCompView(this);
    m_name = new QLabel(this);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->addWidget(m_name);
    vbox->addWidget(m_view);
}

auto SubtitleViewer::CompView::setModel(SubCompModel *model) -> void
{
    if (model)
        m_name->setText(model->name());
    m_view->setModel(model);
}

/******************************************************************************/

struct SubtitleViewer::Data {
    SubtitleViewer *p = nullptr;
    QVector<SubCompModel*> models;
    QList<CompView*> comp;
    QSplitter *splitter;
    QCheckBox *timeVisible, *autoScroll;
    bool needToUpdate = false;
    ObjectStorage storage;
    auto seekIndex(const QModelIndex &idx) -> void
    {
        if (auto model = qobject_cast<const SubCompModel*>(idx.model()))
            emit p->seekRequested(model->at(idx.row()).start());
    };
};

SubtitleViewer::SubtitleViewer(QWidget *parent)
    : QDialog(parent)
    , d(new Data)
{
    d->p = this;

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
            this, &SubtitleViewer::setTimeVisible);
    connect(d->autoScroll, &QCheckBox::toggled,
            this, &SubtitleViewer::setAutoScrollEnabled);

    _SetWindowTitle(this, tr("Subtitle Viewer"));

    d->storage.setObject(this, u"subtitle-viewer"_q, true);
    d->storage.add("time_visible", d->timeVisible, "checked");
    d->storage.add("auto_scroll", d->autoScroll, "checked");
    d->storage.restore();
}

SubtitleViewer::~SubtitleViewer()
{
    delete d;
}

auto SubtitleViewer::updateModels() -> void
{
    if (d->models.isEmpty()) {
        d->splitter->setVisible(false);
        for (int i=0; i<d->comp.size(); ++i)
            d->comp[i]->setModel(nullptr);
    } else  {
        d->splitter->setVisible(true);
        while (d->comp.size() > d->models.size())
            delete d->comp.takeLast();
        while (d->comp.size() < d->models.size()) {
            CompView *comp = new CompView(d->splitter);
            d->splitter->addWidget(comp);
            d->comp.push_back(comp);
            connect(comp->view(), &QTreeView::doubleClicked, this,
                    [=] (auto &idx) { d->seekIndex(idx); });
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

auto SubtitleViewer::setModels(const QVector<SubCompModel*> &models) -> void
{
    d->models = models;
    if (isVisible())
        updateModels();
    else
        d->needToUpdate = true;
}

auto SubtitleViewer::setTimeVisible(bool visible) -> void
{
    for (int i=0; i<d->comp.size(); ++i)
        d->comp[i]->view()->setTimeVisible(visible);
}

auto SubtitleViewer::setAutoScrollEnabled(bool enabled) -> void
{
    for (int i=0; i<d->comp.size(); ++i)
        d->comp[i]->view()->setAutoScrollEnabled(enabled);
}

auto SubtitleViewer::showEvent(QShowEvent *event) -> void
{
    QDialog::showEvent(event);
    if (d->needToUpdate)
        updateModels();
}
