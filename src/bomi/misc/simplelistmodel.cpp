#include "simplelistmodel.hpp"
#include <set>

#define EMIT_DATA_CHANGED(...) emit QAbstractListModel::dataChanged(__VA_ARGS__)

struct SimpleListModelBase::Data {
    int columns = 1, special = -1, rows = 0;
    QFont specialFont;
    QHash<int, QVector<bool>> checked;
};

SimpleListModelBase::SimpleListModelBase(int columns, QObject *parent)
    : QAbstractListModel(parent), d(new Data)
{
    d->columns = columns;
    connect(this, &QAbstractItemModel::dataChanged,
            this, [this] (const QModelIndex &tl, const QModelIndex &br)
    {
        emit rowChanged(tl.row());
        if (tl.row() != br.row())
            emit rowChanged(br.row());
    });

    connect(this, &SimpleListModelBase::dataChanged, this,
            [=] (int row, const QVector<int> &roles)
    {
        if (!isValidRow(row))
            return;
        const auto tl = index(row, 0), br = index(row, d->columns - 1);
        emit QAbstractListModel::dataChanged(tl, br, roles);
    });

    auto signal = &SimpleListModelBase::contentsChanged;
    using M = QAbstractItemModel;
    connect(this, &M::modelReset, this, signal);
    connect(this, &M::dataChanged, this, signal);
    connect(this, &M::rowsInserted, this, signal);
    connect(this, &M::rowsRemoved, this, signal);
    connect(this, &M::rowsMoved, this, signal);
    connect(this, &M::layoutChanged, this, signal);
}

SimpleListModelBase::~SimpleListModelBase()
{
    delete d;
}

auto SimpleListModelBase::columnCount(const QModelIndex &) const -> int
{
    return d->columns;
}

auto SimpleListModelBase::rowCount(const QModelIndex &parent) const -> int
{
    return parent.isValid() ? 0 : d->rows;
}

auto SimpleListModelBase::rows() const -> int
{
    return d->rows;
}

auto SimpleListModelBase::columns() const -> int
{
    return d->columns;
}

auto SimpleListModelBase::size() const -> int
{
    return d->rows;
}

auto SimpleListModelBase::isEmpty() const -> bool
{
    return d->rows <= 0;
}

auto SimpleListModelBase::isValidRow(int r) const -> bool
{
    return _InRange0(r, d->rows);
}

auto SimpleListModelBase::isValidColumn(int c) const -> bool
{
    return _InRange0(c, columns());
}

auto SimpleListModelBase::isValid(const QModelIndex &index) const -> bool
{
    return isValidRow(index.row()) && isValidColumn(index.column());
}

auto SimpleListModelBase::specialRow() const -> int
{
    return d->special;
}

auto SimpleListModelBase::specialFont() const -> QFont
{
    return d->specialFont;
}

auto SimpleListModelBase::setSpecialFont(const QFont &font) -> void
{
    d->specialFont = font;
}

auto SimpleListModelBase::clear() -> void
{
    if (isEmpty())
        return;
    beginResetModel();
    removeAll();
    reset(0);
    endResetModel();
}

auto SimpleListModelBase::insertRows(int row, int count, const QModelIndex &parent) -> bool
{
    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        insertAt(row + i);
        for (auto &v : d->checked)
            v.insert(row + i, false);
    }
    emit rowsChanged(d->rows += count);
    if (d->special >= row)
        setSpecialRow(d->special + count);
    endInsertRows();
    return true;
}

auto SimpleListModelBase::removeRows(int row, int count, const QModelIndex &parent) -> bool
{
    const int to = row + count - 1;
    int removed = 0;
    beginRemoveRows(parent, row, to);
    for (int i = 0; i < count; ++i) {
        if (removeAt(to - i)) {
            ++removed;
            for (auto &v : d->checked)
                v.removeAt(to - i);
        }
    }
    emit rowsChanged(d->rows -= removed);
    if (d->special >= row)
        setSpecialRow(d->special > to ? d->special - removed : -1);
    endRemoveRows();
    return true;
}

auto SimpleListModelBase::headerData(int idx, Qt::Orientation o,
                                    int role) const -> QVariant
{
    if (o == Qt::Horizontal && role == Qt::DisplayRole && isValidColumn(idx))
        return header(idx);
    return QVariant();
}

auto SimpleListModelBase::data(const QModelIndex &idx, int role) const -> QVariant
{
    if (!isValid(idx))
        return QVariant();
    const int row = idx.row(), col = idx.column();
    switch (role) {
    case Qt::DisplayRole:
        return displayData(row, col);
    case Qt::EditRole:
        return editData(row, col);
    case Qt::CheckStateRole: {
        auto it = d->checked.find(col);
        if (it == d->checked.end())
            return QVariant();
        Q_ASSERT(it->size() == d->rows);
        return it->at(row) ? Qt::Checked : Qt::Unchecked;
    } case Qt::FontRole:
        return fontData(row, col);
    default:
        return roleData(row, col, role);
    }
}

auto SimpleListModelBase::setData(const QModelIndex &idx, const QVariant &val,
                                  int role) -> bool
{
    if (!isValid(idx))
        return false;
    if (role == Qt::CheckStateRole)
        return setChecked(idx.row(), idx.column(), val.toBool());
    if (role == Qt::EditRole) {
        const auto res = edit(idx.row(), idx.column(), val);
        if (res)
            EMIT_DATA_CHANGED(idx, idx, QVector<int>());
        return res;
    }
    return false;
}

auto SimpleListModelBase::flags(const QModelIndex &idx) const -> Qt::ItemFlags
{
    return isValid(idx) ? flags(idx.row(), idx.column()) : Qt::NoItemFlags;
}

auto SimpleListModelBase::resize(int rows) -> void
{
    if (_Change(d->rows, rows)) {
        emit rowsChanged(d->rows);
        for (auto &v : d->checked)
            v.resize(d->rows);
        if (d->special >= d->rows)
            setSpecialRow(-1);
    }
}

auto SimpleListModelBase::reset(int rows) -> void
{
    if (_Change(d->rows, rows))
        emit rowsChanged(d->rows);
    for (auto &v : d->checked) {
        v.resize(d->rows);
        v.fill(false);
    }
    setSpecialRow(-1);
}

auto SimpleListModelBase::setSpecialRow(int row) -> void
{
    if (d->special == row)
        return;
    const int prev = d->special;
    d->special = row;
    emit dataChanged(prev, { Qt::FontRole });
    emit dataChanged(d->special, { Qt::FontRole });
    emit specialRowChanged(d->special);
}

auto SimpleListModelBase::checkedList(int column) const -> QVector<bool>
{
    return d->checked.value(column);
}

auto SimpleListModelBase::isChecked(int row, int column) const -> bool
{
    return d->checked.value(column).value(row, false);
}

auto SimpleListModelBase::setChecked(int r, int c, bool checked) -> bool
{
    auto it = d->checked.find(c);
    if (it == d->checked.end() || !_Change((*it)[r], checked))
        return false;
    const auto idx = index(r, c);
    EMIT_DATA_CHANGED(idx, idx, {Qt::CheckStateRole});
    return true;
}

auto SimpleListModelBase::setChecked(int c, const QVector<bool> &checked) -> bool
{
    auto it = d->checked.find(c);
    if (it == d->checked.end() || it->size() != checked.size())
        return false;
    *it = checked;
    const auto tl = index(0, c), br = index(checked.size()-1, c);
    EMIT_DATA_CHANGED(tl, br, {Qt::CheckStateRole});
    return true;
}

auto SimpleListModelBase::setCheckable(int column, bool checkable) -> void
{
    if (checkable)
        d->checked[column].resize(d->rows);
    else
        d->checked.remove(column);
}

auto SimpleListModelBase::flags(int row, int column) const -> Qt::ItemFlags
{
    Q_UNUSED((row | column));
    auto flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (d->checked.contains(column))
        flags |= Qt::ItemIsUserCheckable;
    return flags;
}

auto SimpleListModelBase::header(int column) const -> QString
{
    Q_UNUSED(column);
    return QString();
}

auto SimpleListModelBase::displayData(int r, int c) const -> QVariant
{
    Q_UNUSED((r | c));
    return QVariant();
}

auto SimpleListModelBase::fontData(int r, int c) const -> QFont
{
    Q_UNUSED((r | c));
    return r == specialRow() ? specialFont() : QFont();
}

auto SimpleListModelBase::roleData(int r, int c, int role) const -> QVariant
{
    Q_UNUSED((r | c | role));
    return QVariant();
}

auto SimpleListModelBase::edit(int r, int c, const QVariant &var) -> bool
{
    Q_UNUSED((r | c)); Q_UNUSED(var);
    return false;
}

auto SimpleListModelBase::editData(int row, int column) const -> QVariant
{
    return displayData(row, column);
}

auto SimpleListModelBase::remove(int row) -> bool
{
    return isValidRow(row) ? removeRow(row) : false;
}

auto SimpleListModelBase::remove(const QModelIndexList &indices) -> int
{
    std::set<int> set;
    for (auto &index : indices) {
        if (isValidRow(index.row()))
            set.insert(index.row());
    }
    if (set.empty())
        return 0;
    for (auto it = set.rbegin(); it != set.rend(); ++it)
        removeRow(*it);
    return set.size();
}

auto SimpleListModelBase::swap(int r1, int r2) -> bool
{
    if (!swapAt(r1, r2))
        return false;
    for (auto &v : d->checked)
        std::swap(v[r1], v[r2]);
    emit dataChanged(r1);
    emit dataChanged(r2);
    if (r1 == d->special)
        setSpecialRow(r2);
    else if (r2 == d->special)
        setSpecialRow(r1);
    return true;
}
