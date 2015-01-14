#ifndef HISTORYMODEL_HPP
#define HISTORYMODEL_HPP

#include "mrlstate.hpp"

class HistoryModel: public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
public:
    enum Role {NameRole = Qt::UserRole + 1, LatestPlayRole, LocationRole};
    HistoryModel(QObject *parent = nullptr);
    ~HistoryModel();
    auto rowCount(const QModelIndex &parent = QModelIndex()) const -> int;
    auto columnCount(const QModelIndex &parent = QModelIndex()) const -> int;
    auto data(const QModelIndex &index,
              int role = Qt::DisplayRole) const -> QVariant;
    auto error() const -> QSqlError;
    auto roleNames() const -> QHash<int, QByteArray>;
    auto find(const Mrl &mrl) const -> const MrlState*;
    auto getState(MrlState *state) const -> bool;
    auto update(const MrlState *state, bool reload) -> void;
    auto setRememberImage(bool on) -> void;
    auto setPropertiesToRestore(const QVector<QMetaProperty> &properties) -> void;
    auto clear() -> void;
    auto isVisible() const -> bool;
    auto setVisible(bool visible) -> void;
    auto toggle() -> void { setVisible(!isVisible()); }
    Q_INVOKABLE void play(int row);
signals:
    void playRequested(const Mrl &mrl);
    void changeVisibilityRequested(bool visible);
    void visibleChanged(bool visible);
private:
    struct Data;
    Data *d;
};

#endif // HISTORYMODEL_HPP
