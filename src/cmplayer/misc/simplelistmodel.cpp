#include "simplelistmodel.hpp"

SimpleListModelBase::SimpleListModelBase(int columns, QObject *parent)
    : QAbstractListModel(parent), m_columns(columns)
{
    connect(this, &QAbstractItemModel::dataChanged,
            this, [this] (const QModelIndex &tl, const QModelIndex &br)
    {
        emit rowChanged(tl.row());
        if (tl.row() != br.row())
            emit rowChanged(br.row());
    });
}
