#include "subtitlemodel.hpp"
#include "misc/matchstring.hpp"
#include <QScrollBar>
#include <QSortFilterProxyModel>

SIA operator < (int lhs, const SubCompModelData &rhs) -> bool
{
    return lhs < rhs.start();
}

struct SubCompModel::Data {
    bool visible = false, ms = false, fps = false;
    QString name;
};

SubCompModel::SubCompModel(QObject *parent)
    : Super(ColumnCount, parent)
    , d(new Data)
{
    QFont font; font.setBold(true); font.setItalic(true);
    setSpecialFont(font);
}

auto SubCompModel::setComponent(const SubComp &comp) -> void
{
    d->name = comp.name();
    d->fps = comp.isBasedOnFrame();

    auto it = comp.begin();
    QList<SubCompModelData> list;
    for (; it != comp.end(); ++it) {
        if (it->hasWords()) {
            it->index = 0;
            list.append(it);
            break;
        }
    }
    if (!list.isEmpty()) {
        for (++it; it != comp.end(); ++it) {
            auto &last = list.last();
            if (last.m_end < 0)
                last.m_end = it.key();
            if (it->hasWords())
                list.append(it);
            it->index = list.size() - 1;
        }
    }
    setList(list);
}

auto SubCompModel::header(int column) const -> QString
{
    switch (column) {
    case Start: return tr("Start");
    case End:   return tr("End");
    case Text:  return tr("Text");
    default:    return QString();
    }
}

auto SubCompModel::setTimeInMilliseconds(bool ms) -> void
{
    if (d->ms == ms)
        return;
    beginResetModel();
    d->ms = ms;
    endResetModel();
}

auto SubCompModel::displayData(int row, int column) const -> QVariant
{
    auto &data = at(row);
    switch (column) {
    case Start: return d->ms ? _N(data.start()) : _MSecToString(data.start());
    case End:   return d->ms ? _N(data.end())   : _MSecToString(data.end());
    case Text:  return data.text();
    default:    return QVariant();
    }
}

auto SubCompModel::name() const -> QString
{
    return d->name;
}

auto SubCompModel::setFps(double fps) -> void
{
    if (d->fps) {
        beginResetModel();
        const auto mul = 1000.0/fps;
        for (auto &data : getList())
            data.m_mul = mul;
        endResetModel();
    }
}

auto SubCompModel::setVisible(bool visible) -> void
{
    if (d->visible != visible)
        d->visible = visible;
}

auto SubCompModel::setCurrentCaption(int time) -> void
{
    auto &l = this->list();
    setSpecialRow((std::upper_bound(l.begin(), l.end(), time) - l.begin()) - 1);
}

/******************************************************************************/

class SubCompFilterModel : public QSortFilterProxyModel {
    auto filterAcceptsRow(int srow, const QModelIndex &) const -> bool final
    {
        auto m = static_cast<SubCompModel*>(sourceModel());
        if (start >= 0 && m->at(srow).start() < start)
            return false;
        if (end >= 0 && m->at(srow).start() > end)
            return false;
        if (caption.string().isEmpty() || !caption.isValid())
            return true;
        return caption.contains(m->at(srow).text());
    }
public:
    int start = -1, end = -1;
    MatchString caption;
};

struct SubCompView::Data {
    SubCompView *p = nullptr;
    SubCompModel *model = nullptr;
    SubCompFilterModel proxy;
    bool autoScroll = false;
    bool ms = false, time = true;
    auto updateHeader()
    {
        p->setColumnHidden(SubCompModel::Start, !time);
        p->setColumnHidden(SubCompModel::End, !time);
        if (model)
            model->setTimeInMilliseconds(ms);
    }
};

SubCompView::SubCompView(QWidget *parent)
    : QTreeView(parent)
    , d(new Data)
{
    d->p = this;
    setAlternatingRowColors(true);
    setRootIsDecorated(false);
    setHorizontalScrollMode(ScrollPerPixel);
    setAutoScroll(false);
    d->updateHeader();
    QTreeView::setModel(&d->proxy);
}

auto SubCompView::adjustColumns() -> void
{
    for (int i=0; i<SubCompModel::ColumnCount; ++i)
        resizeColumnToContents(i);
}

auto SubCompView::setModelToNull() -> void
{
    d->proxy.setSourceModel(nullptr);
}

auto SubCompView::setModel(QAbstractItemModel *model) -> void
{
    if (d->model)
        d->model->disconnect(this);
    d->model = dynamic_cast<SubCompModel*>(model);
    d->proxy.setSourceModel(d->model);
    if (d->model) {
        d->model->setTimeInMilliseconds(d->ms);
        d->model->setVisible(isVisible());
        connect(d->model, &SubCompModel::destroyed,
                this, &SubCompView::setModelToNull, Qt::DirectConnection);
        connect(d->model, &SubCompModel::specialRowChanged,
                this, &SubCompView::updateCurrentRow);
    }
}

auto SubCompView::updateCurrentRow(int row) -> void
{
    if (!d->model || !d->autoScroll)
        return;
    auto m = static_cast<SubCompModel*>(d->proxy.sourceModel());
    if (!m)
        return;
    const QModelIndex idx = d->proxy.mapFromSource(m->index(row, SubCompModel::Text));
    if (idx.isValid()) {
        auto h = horizontalScrollBar();
        const int prev = h->value();
        scrollTo(idx);
        h->setValue(prev); // restore previous horizontal scroll bar position
    }
}

auto SubCompView::setAutoScrollEnabled(bool enabled) -> void
{
    if (d->autoScroll != enabled) {
        d->autoScroll = enabled;
        if (d->autoScroll && d->model)
            updateCurrentRow(d->model->specialRow());
    }
}

auto SubCompView::setTimeVisible(bool visible) -> void
{
    if (_Change(d->time, visible))
        d->updateHeader();
}

auto SubCompView::showEvent(QShowEvent *event) -> void
{
    QTreeView::showEvent(event);
    if (d->model)
        d->model->setVisible(true);
}

auto SubCompView::hideEvent(QHideEvent *event) -> void
{
    QTreeView::hideEvent(event);
    if (d->model)
        d->model->setVisible(false);
}

auto SubCompView::setFilter(int start, int end, const MatchString &caption) -> void
{
    d->proxy.start = start;
    d->proxy.end = end;
    d->proxy.caption = caption;
    d->proxy.invalidate();
}

auto SubCompView::setTimeInMilliseconds(bool ms) -> void
{
    if (_Change(d->ms, ms))
        d->updateHeader();
}
