#ifndef HISTORYMODEL_HPP
#define HISTORYMODEL_HPP

#include "stdafx.hpp"
#include "mrlstate.hpp"

class HistoryModel: public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
public:
    enum Role {NameRole = Qt::UserRole + 1, LatestPlayRole, LocationRole};
    HistoryModel(QObject *parent = nullptr);
    ~HistoryModel();
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QSqlError error() const;
    QHash<int, QByteArray> roleNames() const;
    const MrlState *find(const Mrl &mrl) const;
    bool getState(MrlState *state) const;
    void update(const MrlState *state, bool reload);
    Q_INVOKABLE void play(int row);
    void setRememberImage(bool on);
    void setPropertiesToRestore(const QList<QMetaProperty> &properties);
    void getAppState(MrlState *appState);
    void setAppState(const MrlState *appState);
    void clear();
    bool isVisible() const;
    void setVisible(bool visible);
    void toggle() { setVisible(!isVisible()); }
signals:
    void playRequested(const Mrl &mrl);
    void changeVisibilityRequested(bool visible);
    void visibleChanged(bool visible);
private:
    struct Data;
    Data *d;
};

#endif // HISTORYMODEL_HPP
