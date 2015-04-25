#include "subtitleviewer.hpp"
#include "subtitlemodel.hpp"
#include "subtitle.hpp"
#include "misc/matchstring.hpp"
#include "misc/objectstorage.hpp"
#include "ui_subtitleviewer.h"
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QScrollArea>

class SubtitleViewer::CompView : public QWidget {
public:
    CompView(QWidget *parent = nullptr);
    auto setComponent(const SubComp &comp) -> void;
    auto view() const -> SubCompView* {return m_view;}
    auto setCurrentTime(int time) -> void { m_model.setCurrentCaption(time); }
private:
    SubCompView *m_view;
    SubCompModel m_model;
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

    m_view->setModel(&m_model);
}

auto SubtitleViewer::CompView::setComponent(const SubComp &comp) -> void
{
    m_name->setText(comp.name());
    m_model.setComponent(comp);
}

/******************************************************************************/

struct SubtitleViewer::Data {
    SubtitleViewer *p = nullptr;
    Ui::SubtitleViewer ui;
    QVector<SubComp> comps;
    QList<CompView*> views;
    QSplitter *splitter;
    bool needToUpdate = false;
    ObjectStorage storage;
    QRegEx rxFormatTime, rxMSec;
    QString toolTip;
    Seek seek;
    int lastTime = -1;

    static constexpr int InvalidFormat = -1;
    static constexpr int EmptyText = -2;

    auto time(const QLineEdit *edit) -> int
    {
        const auto text = edit->text();
        if (text.isEmpty())
            return EmptyText;
        auto m = rxFormatTime.match(text);
        if (m.hasMatch()) {
            const int hh = m.capturedRef(1).toInt();
            const int mm = m.capturedRef(2).toInt();
            const int ms = m.capturedRef(3).toDouble() * 1000;
            return (hh * 60 + mm) * 60 * 1000 + ms;
        }
        m = rxMSec.match(text);
        if (m.hasMatch())
            return m.capturedRef(0).toInt();
        return InvalidFormat;
    }

#define SET_ALL(func, ...) setAll(&SubCompView::func, __VA_ARGS__)

    template<class R, class... Args, class... Values>
    auto setAll(R(SubCompView::*set)(Args...), Values... args) -> void
        { for (auto v : views) (v->view()->*set)(args...); }

    auto filter() -> void
    {
        const int start = time(ui.start);
        const int end = time(ui.end);
        MatchString caption;
        caption.setCaseSensitive(ui.case_sensitive->isChecked());
        caption.setRegEx(ui.regex->isChecked());
        caption.setString(ui.caption->text());
        SET_ALL(setFilter, start, end, caption);
    }
    auto validate(QLineEdit *edit) -> void
    {
        auto p = ui.caption->palette();
        const QString text = edit->text();
        if (!text.isEmpty() && !rxFormatTime.match(text).hasMatch()
                && !rxMSec.match(text).hasMatch())
            p.setColor(QPalette::Text, Qt::red);
        edit->setPalette(p);
    }
    auto seekIndex(const QModelIndex &idx) -> void
    {
        auto proxy = qobject_cast<const QSortFilterProxyModel*>(idx.model());
        if (!proxy)
            return;
        const auto src = proxy->mapToSource(idx);
        auto model = static_cast<const SubCompModel*>(src.model());
        if (model && src.isValid() && seek)
            seek(model->at(src.row()).start());
    }
};

SubtitleViewer::SubtitleViewer(QWidget *parent)
    : QDialog(parent)
    , d(new Data)
{
    d->p = this;
    d->rxFormatTime = QRegEx(uR"(^(\d+):([0-5]?\d):([0-5]?\d(\.\d*)?)$)"_q);
    d->rxMSec = QRegEx(uR"(^\d+$)"_q);
    d->toolTip = tr("Next formats are allowed:\n"
                    "Decimal digits only\n - translated into milliseconds\n"
                    "h:m:s\n - hours:min:sec where s can contain decimal point.");

    d->ui.setupUi(this);
    d->ui.bbox->button(BBox::Close)->setAutoDefault(false);
    delete d->ui.area->takeWidget();
    d->splitter = new QSplitter(Qt::Horizontal, d->ui.area);
    d->ui.area->setWidget(d->splitter);

    connect(d->ui.time_visible, &QCheckBox::toggled, this,
            [=] (bool visible) { d->SET_ALL(setTimeVisible, visible); });
    connect(d->ui.autoscroll, &QCheckBox::toggled, this,
            [=] (bool as) { d->SET_ALL(setAutoScrollEnabled, as); });
    connect(d->ui.start, &QLineEdit::textEdited, this, [=] () { d->validate(d->ui.start); });
    connect(d->ui.end,   &QLineEdit::textEdited, this, [=] () { d->validate(d->ui.end); });

    auto filter = [=] () { d->filter(); };
    connect(d->ui.start, &QLineEdit::editingFinished, this, filter);
    connect(d->ui.end,   &QLineEdit::editingFinished, this, filter);
    connect(d->ui.caption, &QLineEdit::textChanged, this, filter);
    connect(d->ui.regex, &QCheckBox::toggled, this, filter);
    connect(d->ui.case_sensitive, &QCheckBox::toggled, this, filter);
    connect(d->ui.time_ms, &QCheckBox::toggled, this,
            [=] (bool ms) { d->SET_ALL(setTimeInMilliseconds, ms); });
    d->ui.start->setToolTip(d->toolTip);
    d->ui.end->setToolTip(d->toolTip);

    _SetWindowTitle(this, tr("Subtitle Viewer"));

    d->storage.setObject(this, u"subtitle-viewer"_q);
    d->storage.add(d->ui.autoscroll);
    d->storage.add(d->ui.time_visible);
    d->storage.add(d->ui.time_ms);
    d->storage.add(d->ui.regex);
    d->storage.add(d->ui.case_sensitive);
    d->storage.add(d->ui.start);
    d->storage.add(d->ui.end);
    d->storage.add(d->ui.caption);
    d->storage.restore();
}

SubtitleViewer::~SubtitleViewer()
{
    d->storage.save();
    delete d;
}

auto SubtitleViewer::updateModels() -> void
{
    if (d->comps.isEmpty()) {
        d->splitter->setVisible(false);
    } else  {
        d->splitter->setVisible(true);
        while (d->views.size() > d->comps.size())
            delete d->views.takeLast();
        while (d->views.size() < d->comps.size()) {
            CompView *comp = new CompView(d->splitter);
            d->splitter->addWidget(comp);
            d->views.push_back(comp);
            connect(comp->view(), &QTreeView::doubleClicked, this,
                    [=] (auto &idx) { d->seekIndex(idx); });
        }
        for (int i=0; i<d->views.size(); ++i) {
            d->views[i]->setComponent(d->comps[i]);
            d->views[i]->setCurrentTime(d->lastTime);
            const auto v = d->views[i]->view();
            v->setAutoScrollEnabled(d->ui.autoscroll->isChecked());
            v->setTimeVisible(d->ui.time_visible->isChecked());
            v->setTimeInMilliseconds(d->ui.time_ms->isChecked());
            v->adjustColumns();
        }
        d->comps.clear();
        d->filter();
    }
    d->needToUpdate = false;
}

auto SubtitleViewer::setCurrentTime(int time) -> void
{
    if (_Change(d->lastTime, time)) {
        for (auto v : d->views)
            v->setCurrentTime(time);
    }
}

auto SubtitleViewer::setComponents(const QVector<SubComp> &comps) -> void
{
    d->comps = comps;
    if (isVisible())
        updateModels();
    else
        d->needToUpdate = true;
}

auto SubtitleViewer::showEvent(QShowEvent *event) -> void
{
    QDialog::showEvent(event);
    if (d->needToUpdate)
        updateModels();
}

auto SubtitleViewer::setSeekFunc(Seek &&func) -> void
{
    d->seek = std::move(func);
}
