#ifndef SIMPLELISTMODEL_HPP
#define SIMPLELISTMODEL_HPP

#include "stdafx.hpp"

template<typename T> class SimpleListModel;

class SimpleListModelBase : public QAbstractListModel {
    Q_OBJECT
public:
    using QAbstractListModel::QAbstractListModel;
private:
    void emitDataChanged(const QModelIndex &index, int role) {
        emit dataChanged(index, index, QVector<int>() << role);
    }
    void emitDataChanged(const QModelIndex &tl, const QModelIndex &br, int role) {
        emit dataChanged(tl, br, QVector<int>() << role);
    }
    template<typename T> friend class SimpleListModel;
};

template<typename T>
class SimpleListModel : public SimpleListModelBase {
public:
    SimpleListModel(QObject *parent = nullptr): SimpleListModelBase(parent) { }
    SimpleListModel(int columns, QObject *parent = nullptr): SimpleListModelBase(parent), m_columns(columns) { }
    int rowCount(const QModelIndex &parent = QModelIndex()) const final { return parent.isValid() ? 0 : m_list.size(); }
    int columnCount(const QModelIndex &parent = QModelIndex()) const final { Q_UNUSED(parent); return m_columns; }
    void setList(const QList<T> &list) {
        beginResetModel();
        m_list = list;
        for (auto &it : m_checked)
            it.resize(m_list.size());
        endResetModel();
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const final {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole || !_InRange(0, section, m_columns-1))
            return QVariant();
        return headerText(section);
    }
    QVariant data(const QModelIndex &index, int role) const final {
        if (!isValid(index))
            return QVariant();
        const int row = index.row(), col = index.column();
        if (role == Qt::DisplayRole)
            return displayData(row, col);
        if (role == Qt::CheckStateRole) {
            auto it = m_checked.find(col);
            if (it == m_checked.end())
                return QVariant();
            Q_ASSERT(it->size() == m_list.size());
            return it->at(row) ? Qt::Checked : Qt::Unchecked;
        }
        if (role >= Qt::UserRole)
            return roleData(row, col, role);
        return QVariant();
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role) final {
        if (!isValid(index))
            return false;
        if (role == Qt::CheckStateRole)
            return setChecked(index.row(), index.column(), value.toBool());
        return false;
    }
    Qt::ItemFlags flags(const QModelIndex &index) const final override {
        if (isValid(index))
            return flags(index.row(), index.column());
        return Qt::NoItemFlags;
    }
    const T &at(int row) const { return m_list.at(row); }
    T value(int row) const { return m_list.value(row); }
    int size() const { return m_list.size(); }
    const QList<T> &list() const { return m_list; }
    bool setChecked(int row, int column, bool checked) {
        auto it = m_checked.find(column);
        if (it == m_checked.end() || !_Change((*it)[row], checked))
            return false;
        emitDataChanged(this->index(row, column), Qt::CheckStateRole);
        return true;
    }
    bool setChecked(int column, const QVector<bool> &checked) {
        auto it = m_checked.find(column);
        if (it == m_checked.end() || it->size() != checked.size())
            return false;
        *it = checked;
        emitDataChanged(index(0, column), index(checked.size()-1, column), Qt::CheckStateRole);
        return true;
    }
    QVector<bool> checkedList(int column) const { return m_checked.value(column); }
protected:
    void setCheckable(int column, bool checkable) {
        if (checkable)
            m_checked[column].resize(m_list.size());
        else
            m_checked.remove(column);
    }
    typedef SimpleListModel<T> Super;
    bool isValid(const QModelIndex &index) const {
        return _InRange(0, index.row(), m_list.size()-1) && _InRange(0, index.column(), m_columns-1);
    }
    virtual Qt::ItemFlags flags(int row, int column) const { Q_UNUSED((row | column)); return Qt::ItemIsEnabled | Qt::ItemIsSelectable; }
    virtual QVariant headerText(int column) const { Q_UNUSED(column); return QVariant(); }
    virtual QVariant displayData(int row, int column) const { Q_UNUSED((row | column)); return QVariant(); }
    virtual QVariant roleData(int row, int column, int role) const { Q_UNUSED((row | column | role)); return QVariant(); }
    virtual QVariant checkStateData(int row, int column) const { Q_UNUSED((row | column)); return QVariant(); }
private:
    int m_columns = 1;
    QList<T> m_list;
    QHash<int, QVector<bool>> m_checked;
};

#endif // SIMPLELISTMODEL_HPP
