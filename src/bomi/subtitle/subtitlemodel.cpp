#include "subtitlemodel.hpp"
#include <QScrollBar>

struct SubCompModel::Data {
    bool visible = false;
    const SubComp *comp = nullptr;
    const SubCapt *pended = nullptr;
};

SubCompModel::SubCompModel(const SubComp *comp, QObject *parent)
    : Super(ColumnCount, parent)
    , d(new Data)
{
    d->comp = comp;
    QFont font; font.setBold(true); font.setItalic(true);
    setSpecialFont(font);

    auto it = comp->begin();
    QList<SubCompModelData> list;
    for (; it != comp->end(); ++it) {
        if (it->hasWords()) {
            it->index = 0;
            list.append(it);
            break;
        }
    }
    if (!list.isEmpty()) {
        for (++it; it != comp->end(); ++it) {
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

auto SubCompModel::displayData(int row, int column) const -> QVariant
{
    auto &data = at(row);
    switch (column) {
    case Start: return _MSecToString(data.start());
    case End:   return _MSecToString(data.end());
    case Text:  return data.m_it->toPlainText();
    default:    return QVariant();
    }
}

auto SubCompModel::name() const -> QString
{
    return d->comp->name();
}

auto SubCompModel::setFps(double fps) -> void
{
    if (d->comp->isBasedOnFrame()) {
        beginResetModel();
        const auto mul = 1000.0/fps;
        for (auto &data : getList())
            data.m_mul = mul;
        endResetModel();
    }
}

auto SubCompModel::setVisible(bool visible) -> void
{
    if (d->visible != visible) {
        d->visible = visible;
        if (d->visible && d->pended)
            setCurrentCaption(d->pended);
    }
}

auto SubCompModel::setCurrentCaption(const SubCapt *caption) -> void
{
    if (!d->visible) {
        d->pended = caption;
    } else {
        d->pended = 0;
        setSpecialRow(caption ? caption->index : -1);
    }
}

/******************************************************************************/

struct SubCompView::Data {
    SubCompModel *model = nullptr;
    bool autoScroll = false;
};

SubCompView::SubCompView(QWidget *parent)
    : QTreeView(parent)
    , d(new Data)
{
    setAlternatingRowColors(true);
    setRootIsDecorated(false);
    setHorizontalScrollMode(ScrollPerPixel);
    setAutoScroll(false);
}

auto SubCompView::setModelToNull() -> void
{
    if (sender() == d->model)
        d->model = nullptr;
}

auto SubCompView::setModel(QAbstractItemModel *model) -> void
{
    if (d->model)
        d->model->disconnect(this);
    d->model = qobject_cast<SubCompModel*>(model);
    QTreeView::setModel(d->model);
    if (d->model) {
        connect(d->model, &SubCompModel::destroyed,
                this, &SubCompView::setModelToNull);
        connect(d->model, &SubCompModel::specialRowChanged,
                this, &SubCompView::updateCurrentRow);
        d->model->setVisible(isVisible());
        for (int i=0; i<SubCompModel::ColumnCount; ++i)
            resizeColumnToContents(i);
    }
}

auto SubCompView::updateCurrentRow(int row) -> void
{
    if (!d->model || !d->autoScroll)
        return;
    const QModelIndex idx = d->model->index(row, SubCompModel::Text);
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
    setColumnHidden(SubCompModel::Start, !visible);
    setColumnHidden(SubCompModel::End, !visible);
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
