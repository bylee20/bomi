#ifndef SubCompModel_HPP
#define SubCompModel_HPP

#include "subtitle.hpp"
#include "misc/simplelistmodel.hpp"
#include <QTreeView>

class MatchString;

struct SubCompModelData {
    SubCompModelData() = default;
    SubCompModelData(SubComp::const_iterator it)
        : m_start(it.key()), m_end(-1), m_text(it->toPlainText()) {}
    auto start() const -> int { return m_start * m_mul; }
    auto end() const -> int { return m_end * m_mul; }
    auto text() const -> QString { return m_text; }
private:
    int m_start = -1, m_end = -1;
    double m_mul = 1.0;
    QString m_text;
    friend class SubCompModel;
};

class SubCompModel : public SimpleListModel<SubCompModelData> {
    Q_DECLARE_TR_FUNCTIONS(SubCompModel)
public:
    enum Column {Start = 0, End, Text, ColumnCount};
    SubCompModel(QObject *parent = 0);
    auto name() const -> QString;
    auto setFps(double fps) -> void;
    auto setCurrentCaption(int time) -> void;
    auto setVisible(bool visible) -> void;
    auto setTimeInMilliseconds(bool ms) -> void;
    auto setComponent(const SubComp &comp) -> void;
private:
    auto header(int column) const -> QString final;
    auto displayData(int row, int column) const -> QVariant final;
    struct Data;
    Data *d;
};

class SubCompView : public QTreeView {
    Q_DECLARE_TR_FUNCTIONS(SubCompView)
public:
    SubCompView(QWidget *parent = 0);
    auto setModel(QAbstractItemModel *model) -> void;
    auto setAutoScrollEnabled(bool enabled) -> void;
    auto setTimeVisible(bool visible) -> void;
    auto setTimeInMilliseconds(bool ms) -> void;
    auto setFilter(int start, int end, const MatchString &caption) -> void;
    auto adjustColumns() -> void;
private:
    auto updateCurrentRow(int row) -> void;
    auto setModelToNull() -> void;
    auto showEvent(QShowEvent *event) -> void;
    auto hideEvent(QHideEvent *event) -> void;
    struct Data;
    Data *d;
};

#endif // SUBTITLEMODEL_HPP
