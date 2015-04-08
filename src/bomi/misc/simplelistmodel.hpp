#ifndef SIMPLELISTMODEL_HPP
#define SIMPLELISTMODEL_HPP

template<class T, class List> class SimpleListModel;

class SimpleListModelBase : public QAbstractListModel {
    Q_OBJECT
public:
    SimpleListModelBase(int columns, QObject *parent = nullptr);
    ~SimpleListModelBase();
    auto rows() const -> int;
    auto columns() const -> int;
    auto size() const -> int;
    auto isEmpty() const -> bool;
    auto isValid(const QModelIndex &index) const -> bool;
    auto isValidRow(int r) const -> bool;
    auto isValidColumn(int c) const -> bool;
    auto specialRow() const -> int;
    auto specialFont() const -> QFont;
    auto setChecked(int row, int column, bool checked) -> bool;
    auto setChecked(int column, const QVector<bool> &checked) -> bool;
    auto setCheckable(int column, bool checkable) -> void;
    auto checkedList(int column) const -> QVector<bool>;
    auto isChecked(int row, int column) const -> bool;
    auto remove(int row) -> bool;
    auto remove(const QModelIndexList &indices) -> int;
    auto swap(int r1, int r2) -> bool;
    auto clear() -> void;
    virtual auto header(int column) const -> QString;
signals:
    void rowChanged(int row);
    void rowsChanged(int rows);
    void specialRowChanged(int row);
    void dataChanged(int row, const QVector<int> &roles = QVector<int>());
    void contentsChanged();
protected:
    auto resize(int rows) -> void;
    auto reset(int rows) -> void;
    auto setSpecialFont(const QFont &font) -> void;
    auto setSpecialRow(int row) -> void;
    virtual auto flags(int row, int column) const -> Qt::ItemFlags;
    virtual auto displayData(int row, int column) const -> QVariant;
    virtual auto roleData(int row, int column, int role) const -> QVariant;
    virtual auto fontData(int row, int column) const -> QFont;
    virtual auto editData(int row, int column) const -> QVariant;
    virtual auto edit(int row, int column, const QVariant &var) -> bool;
private:
    virtual auto removeAll() -> void = 0;
    virtual auto insertAt(int row) -> void = 0;
    virtual auto removeAt(int row) -> bool = 0;
    virtual auto swapAt(int r1, int r2) -> bool = 0;
    auto columnCount(const QModelIndex &) const -> int final;
    auto rowCount(const QModelIndex &parent = QModelIndex()) const -> int final;
    auto headerData(int i, Qt::Orientation o, int role) const -> QVariant final;
    auto data(const QModelIndex &idx, int role) const -> QVariant final;
    auto setData(const QModelIndex &idx, const QVariant &value, int role) -> bool final;
    auto flags(const QModelIndex &idx) const -> Qt::ItemFlags final;
    auto insertRows(int row, int count, const QModelIndex &parent) -> bool;
    auto removeRows(int row, int count, const QModelIndex &parent) -> bool;
    struct Data;
    Data *d;
};

DECL_PLUG_CHANGED(SimpleListModelBase, contentsChanged)

template<class T, class Container = QList<T>>
class SimpleListModel : public SimpleListModelBase {
public:
    using Super = SimpleListModel<T, Container>;
    using value_type = T;
    using container_type = Container;
    SimpleListModel(QObject *parent = nullptr)
        : SimpleListModelBase(1, parent) { }
    SimpleListModel(int columns, QObject *parent = nullptr)
        : SimpleListModelBase(columns, parent) { }
    auto at(int row) const -> const T& { return m_list.at(row); }
    auto value(int row) const -> T { return m_list.value(row); }
    auto list() const -> const Container& { return m_list; }
    auto setList(const Container &list) -> void;
    auto append(const T &t) -> void { append(Container() << t); }
    auto append(const Container &list) -> void;
    auto rowOf(const T &t) const -> int {return m_list.indexOf(t);}
protected:
    auto getList() -> Container& { return m_list; }
    auto get(int r) -> T& { return m_list[r]; }
private:
    auto swapAt(int r1, int r2) -> bool final
    {
        if (!isValidRow(r1) || !isValidRow(r2) || r1 == r2)
            return false;
        std::swap(m_list[r1], m_list[r2]);
        return true;
    }
    auto insertAt(int row) -> void final { m_list.insert(row, T()); }
    auto removeAt(int row) -> bool final
        { return _InRange0(row, m_list.size()) && (m_list.removeAt(row), true); }
    auto removeAll() -> void final { m_list.clear(); }
    Container m_list;
};

template<class T, class List>
auto SimpleListModel<T, List>::setList(const List &list) -> void
{
    beginResetModel();
    m_list = list;
    reset(m_list.size());
    endResetModel();
}

template<class T, class List>
auto SimpleListModel<T, List>::append(const List &list) -> void{
    if (list.isEmpty())
        return;
    auto at = m_list.size();
    beginInsertRows(QModelIndex(), at, at + list.size() - 1);
    m_list += list;
    resize(m_list.size());
    endInsertRows();
}

class StringListModel : public SimpleListModel<QString, QStringList> {
    Q_OBJECT
public:
    StringListModel(QObject *parent = nullptr): Super(parent) { }
private:
    auto displayData(int row, int) const -> QVariant { return at(row); }
};


#endif // SIMPLELISTMODEL_HPP
