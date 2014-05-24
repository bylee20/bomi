#ifndef SubCompModel_HPP
#define SubCompModel_HPP

#include "subtitle.hpp"
#include "misc/simplelistmodel.hpp"

class SubCompModelData {
    SubCompModelData(): m_end(-1) {}
    SubCompModelData(SubComp::const_iterator it): m_end(-1), m_it(it) {}
    auto start() const -> int { return m_it.key() * m_mul; }
    auto end() const -> int { return m_end * m_mul; }
private:
    int m_end;
    double m_mul = 1.0;
    SubComp::const_iterator m_it;
    friend class SubCompModel;
};

class SubCompModel : public SimpleListModel<SubCompModelData> {
    Q_OBJECT
public:
    enum Column {Start = 0, End, Text, ColumnCount};
    SubCompModel(const SubComp *comp, QObject *parent = 0);
    auto name() const -> QString;
    auto setFps(double fps) -> void;
    auto setCurrentCaption(const SubCapt *caption) -> void;
    auto setVisible(bool visible) -> void;
private:
    auto header(int column) const -> QString final;
    auto displayData(int row, int column) const -> QVariant final;
    struct Data;
    Data *d;
};

class SubCompView : public QTreeView {
    Q_OBJECT
public:
    SubCompView(QWidget *parent = 0);
    auto setModel(QAbstractItemModel *model) -> void;
    auto setAutoScrollEnabled(bool enabled) -> void;
    auto setTimeVisible(bool visible) -> void;
private:
    auto updateCurrentRow(int row) -> void;
    auto setModelToNull() -> void;
    auto showEvent(QShowEvent *event) -> void;
    auto hideEvent(QHideEvent *event) -> void;
    struct Data;
    Data *d;
};

#endif // SUBTITLEMODEL_HPP
