#ifndef HISTORYMODEL_HPP
#define HISTORYMODEL_HPP

#include "mrlstate.hpp"

class QSqlError;

class HistoryModel: public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(int length READ rowCount NOTIFY lengthChanged)
public:
    enum Role {NameRole = Qt::UserRole + 1, LatestPlayRole, LocationRole, StarRole};
    HistoryModel(QObject *parent = nullptr);
    ~HistoryModel();
    auto rowCount(const QModelIndex &parent = QModelIndex()) const -> int;
    auto columnCount(const QModelIndex &parent = QModelIndex()) const -> int;
    auto data(const QModelIndex &index, int role = Qt::DisplayRole) const -> QVariant final;
    auto error() const -> QSqlError;
    auto roleNames() const -> QHash<int, QByteArray>;
    auto find(const Mrl &mrl) const -> const MrlState*;
    auto getState(MrlState *state) const -> bool;
    auto update(const MrlState *state, const QString &column, bool reload) -> void;
    auto update(const MrlState *state, bool reload) -> void;
    auto setShowMediaTitleInName(bool local, bool url) -> void;
    auto setRememberImage(bool on) -> void;
    auto setPropertiesToRestore(const QStringList &properties) -> void;
    auto isRestorable(const char *name) const -> bool;
    auto clear() -> void;
    auto isVisible() const -> bool;
    auto setVisible(bool visible) -> void;
    auto update() -> void;
    auto toggle() -> void { setVisible(!isVisible()); }
    Q_INVOKABLE bool isStarred(int row) const;
    Q_INVOKABLE void setStarred(int row, bool star);
    Q_INVOKABLE void play(int row);
signals:
    void playRequested(const Mrl &mrl);
    void changeVisibilityRequested(bool visible);
    void visibleChanged(bool visible);
    void lengthChanged(int length);
private:
    auto getData(int row, int role) const -> QVariant;
    struct Data;
    Data *d;
};

#endif // HISTORYMODEL_HPP
