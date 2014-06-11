#ifndef SIMPLELISTMODEL_HPP
#define SIMPLELISTMODEL_HPP

template<class T, class List> class SimpleListModel;

class SimpleListModelBase : public QAbstractListModel {
    Q_OBJECT
public:
    inline auto columns() const -> int { return m_columns; }
    auto specialRow() const -> int { return m_special; }
    auto specialFont() const -> QFont { return m_specialFont; }
signals:
    void rowChanged(int row);
    void rowsChanged(int rows);
    void specialRowChanged(int row);
protected:
    auto setSpecialFont(const QFont &font) -> void { m_specialFont = font; }
    auto setSpecialRow(int row) -> void
    { if (_Change(m_special, row)) emit specialRowChanged(m_special); }
    auto emitRowsChanged(int rows) -> void { emit rowsChanged(rows); }
    auto emitDataChanged(int row) -> void
    { emit dataChanged(index(row, 0), index(row, m_columns-1)); }
    auto emitDataChanged(const QModelIndex &index, int role) -> void
    { emitDataChanged(index, index, role); }
    auto emitDataChanged(const QModelIndex &tl,
                         const QModelIndex &br, int role) -> void
    { emit dataChanged(tl, br, QVector<int>() << role); }
private:
    SimpleListModelBase(int columns, QObject *parent = nullptr);
    template<class T, class List> friend class SimpleListModel;
    auto columnCount(const QModelIndex &) const -> int final
    { return m_columns; }
    int m_columns = 1, m_special = -1;
    QFont m_specialFont;
};

template<class T, class List = QList<T>>
class SimpleListModel : public SimpleListModelBase {
public:
    using Super = SimpleListModel<T, List>;
    SimpleListModel(QObject *parent = nullptr)
        : SimpleListModelBase(1, parent) { }
    SimpleListModel(int columns, QObject *parent = nullptr)
        : SimpleListModelBase(columns, parent) { }
    auto rows() const -> int { return m_list.size(); }
    auto colums() const -> int { return m_columns; }
    auto at(int row) const -> const T& { return m_list.at(row); }
    auto value(int row) const -> T { return m_list.value(row); }
    auto size() const -> int { return m_list.size(); }
    auto isEmpty() const -> bool { return m_list.isEmpty(); }
    auto list() const -> const List& { return m_list; }
    auto setChecked(int row, int column, bool checked) -> bool;
    auto setChecked(int column, const QVector<bool> &checked) -> bool;
    auto setCheckable(int column, bool checkable) -> void;
    auto checkedList(int column) const -> QVector<bool>
    { return m_checked.value(column); }
    auto isValid(const QModelIndex &index) const -> bool
    { return isValidRow(index.row()) && isValidColumn(index.column()); }
    auto isValidRow(int row) const -> bool
    { return _InRange0(row, m_list.size()); }
    auto isValidColumn(int column) const -> bool
    { return _InRange0(column, m_columns); }

    auto setList(const List &list) -> void;
    auto append(const T &t) -> void { append(List() << t); }
    auto append(const List &list) -> void;
    auto merge(const List &list) -> void;
    auto remove(int row) -> bool;
    auto remove(const QModelIndexList &indexes) -> int;
    auto rowOf(const T &t) const -> int {return m_list.indexOf(t);}
    auto swap(int r1, int r2) -> bool;
    virtual auto header(int column) const -> QString;
protected:
    virtual auto flags(int row, int column) const -> Qt::ItemFlags;
    virtual auto displayData(int row, int column) const -> QVariant;
    virtual auto roleData(int row, int column, int role) const -> QVariant;
    virtual auto fontData(int row, int column) const -> QFont;
    auto getList() -> List& { return m_list; }
    auto get(int r) -> T& { return m_list[r]; }
private:
    auto rowCount(const QModelIndex &parent = QModelIndex()) const -> int final
    { return parent.isValid() ? 0 : m_list.size(); }
    auto headerData(int i, Qt::Orientation o, int role) const -> QVariant final;
    QVariant data(const QModelIndex &idx, int role) const final;
    bool setData(const QModelIndex &idx, const QVariant &value, int role) final;
    Qt::ItemFlags flags(const QModelIndex &idx) const final
    { return isValid(idx) ? flags(idx.row(), idx.column()) : Qt::NoItemFlags; }
private:
    List m_list;
    QHash<int, QVector<bool>> m_checked;
};

template<class T, class List>
auto SimpleListModel<T, List>::setList(const List &list) -> void
{
    const int old = m_list.size();
    beginResetModel();
    m_list = list;
    for (auto &it : m_checked)
        it.resize(m_list.size());
    endResetModel();
    if (old != m_list.size())
        emitRowsChanged(m_list.size());
    setSpecialRow(-1);
}

template<class T, class List>
auto SimpleListModel<T, List>::setChecked(int r, int c, bool checked) -> bool
{
    auto it = m_checked.find(c);
    if (it == m_checked.end() || !_Change((*it)[r], checked))
        return false;
    emitDataChanged(index(r, c), Qt::CheckStateRole);
    return true;
}

template<class T, class List>
auto SimpleListModel<T, List>::setChecked(int column,
                                    const QVector<bool> &checked) -> bool
{
    auto it = m_checked.find(column);
    if (it == m_checked.end() || it->size() != checked.size())
        return false;
    *it = checked;
    const auto tl = index(0, column);
    const auto br = index(checked.size()-1, column);
    emitDataChanged(tl, br, Qt::CheckStateRole);
    return true;
}

template<class T, class List>
auto SimpleListModel<T, List>::setCheckable(int column, bool checkable) -> void
{
    if (checkable)
        m_checked[column].resize(m_list.size());
    else
        m_checked.remove(column);
}

template<class T, class List>
auto SimpleListModel<T, List>::flags(int row, int column) const -> Qt::ItemFlags
{ Q_UNUSED((row | column)); return Qt::ItemIsEnabled | Qt::ItemIsSelectable; }

template<class T, class List>
auto SimpleListModel<T, List>::header(int column) const -> QString
{ Q_UNUSED(column); return QString(); }

template<class T, class List>
auto SimpleListModel<T, List>::displayData(int r, int c) const -> QVariant
{ Q_UNUSED((r | c)); return QVariant(); }

template<class T, class List>
auto SimpleListModel<T, List>::fontData(int r, int c) const -> QFont
{ Q_UNUSED((r | c)); return r == specialRow() ? specialFont() : QFont(); }

template<class T, class List>
auto SimpleListModel<T, List>::roleData(int r, int c, int role) const -> QVariant
{ Q_UNUSED((r | c | role)); return QVariant(); }

template<class T, class List>
auto SimpleListModel<T, List>::headerData(int idx, Qt::Orientation o,
                                    int role) const -> QVariant
{
    if (o == Qt::Horizontal && role == Qt::DisplayRole && isValidColumn(idx))
        return header(idx);
    return QVariant();
}

template<class T, class List>
auto SimpleListModel<T, List>::data(const QModelIndex &idx,
                                    int role) const -> QVariant
{
    if (!isValid(idx))
        return QVariant();
    const int row = idx.row(), col = idx.column();
    switch (role) {
    case Qt::DisplayRole:
        return displayData(row, col);
    case Qt::CheckStateRole: {
        auto it = m_checked.find(col);
        if (it == m_checked.end())
            return QVariant();
        Q_ASSERT(it->size() == m_list.size());
        return it->at(row) ? Qt::Checked : Qt::Unchecked;
    } case Qt::FontRole:
        return fontData(row, col);
    default:
        if (role >= Qt::UserRole)
            return roleData(row, col, role);
        return QVariant();
    }
}

template<class T, class List>
auto SimpleListModel<T, List>::setData(const QModelIndex &idx,
                                       const QVariant &val, int role) -> bool {
    if (!isValid(idx))
        return false;
    if (role == Qt::CheckStateRole)
        return setChecked(idx.row(), idx.column(), val.toBool());
    return false;
}

template<class T, class List>
auto SimpleListModel<T, List>::append(const List &list) -> void{
    if (list.isEmpty())
        return;
    auto at = m_list.size();
    beginInsertRows(QModelIndex(), at, at + list.size() - 1);
    m_list += list;
    endInsertRows();
    emitRowsChanged(m_list.size());
}

template<class T, class List>
void SimpleListModel<T, List>::merge(const List &list) {
    if (list.isEmpty())
        return;
    beginResetModel();
    if (m_list.isEmpty())
        m_list = list;
    else {
        for (int i=0; i<list.size(); ++i) {
            if (!m_list.contains(list[i]))
                m_list.append(list[i]);
        }
    }
    endResetModel();
    emitRowsChanged(m_list.size());
}

template<class T, class List>
auto SimpleListModel<T, List>::remove(int row) -> bool
{ return isValidRow(row) ? remove(QModelIndexList() << index(row, 0)) : false; }

template<class T, class List>
auto SimpleListModel<T, List>::remove(const QModelIndexList &indices) -> int
{
    std::set<int> set;
    for (auto &index : indices) {
        if (isValidRow(index.row()))
            set.insert(index.row());
    }
    if (set.empty())
        return 0;
    beginResetModel();
    for (auto it = set.rbegin(); it != set.rend(); ++it)
        m_list.removeAt(*it);
    endResetModel();
    emitRowsChanged(m_list.size());
    if (set.count(specialRow()) > 0)
        setSpecialRow(-1);
    return set.size();
}

template<class T, class List>
auto SimpleListModel<T, List>::swap(int r1, int r2) -> bool
{
    if (!isValidRow(r1) || !isValidRow(r2))
        return false;
    if (r1 == r2)
        return true;
    m_list.swap(r1, r2);
    emitDataChanged(r1);
    emitDataChanged(r2);
    if (r1 == specialRow())
        setSpecialRow(r2);
    else if (r2 == specialRow())
        setSpecialRow(r1);
    return true;
}

class StringListModel : public SimpleListModel<QString, QStringList> {
    Q_OBJECT
public:
    StringListModel(QObject *parent): Super(parent) { }
private:
    auto displayData(int row, int) const -> QVariant { return at(row); }
};


#endif // SIMPLELISTMODEL_HPP
