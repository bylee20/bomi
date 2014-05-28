#ifndef SIMPLELISTWIDGET_HPP
#define SIMPLELISTWIDGET_HPP

#include "misc/simplelistmodel.hpp"

class QListWidgetItem;

template<class T, class List> class SimpleListWidget;

class SimpleListWidgetBase : public QWidget {
    Q_OBJECT
public:
    auto setAddingAndErasingEnabled(bool enabled) -> void;
    auto setChangingOrderEnabled(bool enabled) -> void;
    auto isChangingOrderEnabled() const -> bool { return m_movable; }
    auto isAddingAndErasingEnabled() const -> bool { return m_addable; }
    auto view() const -> QTreeView* { return m_view; }
private:
    SimpleListWidgetBase(QWidget *parent = nullptr);
    QTreeView *m_view;
    QPushButton *m_add, *m_erase, *m_up, *m_down;
    bool m_addable = false, m_movable = false;
    template<class T, class List> friend class SimpleListWidget;
};

template<class T, class List = QList<T>>
class SimpleListWidget : public SimpleListWidgetBase {
public:
    using Super = SimpleListWidget<T, List>;
    using Model = SimpleListModel<T, List>;
    SimpleListWidget(QWidget *parent = 0);
    auto setList(const List &list) -> void
    { if (m_model) m_model->setList(list); }
    auto list() const -> List
    { return m_model ? m_model->list() : List(); }
    auto setModel(Model *model) -> void;
    auto model() const -> Model* { return m_model; }
protected:
    virtual auto getNewItem(T *item) -> bool
    { Q_UNUSED(item); return false; }
private:
    Model *m_model = nullptr;
};

template<class T, class List>
SimpleListWidget<T, List>::SimpleListWidget(QWidget *parent)
    : SimpleListWidgetBase(parent)
{
    m_view->setRootIsDecorated(false);
    setModel(new Model(this));
    auto selection = m_view->selectionModel();
    connect(selection, &QItemSelectionModel::selectionChanged,
            this, [this] (const QItemSelection &selected)
    {
        int min = m_model->rows(), max = -1;
        for (auto idx : selected.indexes()) {
            if (idx.row() < min)
                min = idx.row();
            if (idx.row() > max)
                max = idx.row();
        }
        m_erase->setEnabled(m_addable && max != -1);
        m_up->setEnabled(_InRange(0, max, m_model->rows() - 2));
        m_down->setEnabled(_InRange(1, min, m_model->rows() - 1));
    });
    connect(m_erase, &QPushButton::clicked, this, [this] ()
        { if (m_model) m_model->remove(m_view->currentIndex().row()); });
    auto move = [this] (int diff) {
        if (m_model) {
            const int row = m_view->currentIndex().row();
            m_model->swap(row, row + diff);
        }
    };
    connect(m_up, &QPushButton::clicked, this, [move] () { move(-1); });
    connect(m_down, &QPushButton::clicked, this, [move] () { move(1); });
    connect(m_add, &QPushButton::clicked, this, [this] ()
        { T t; if (m_model && getNewItem(&t)) m_model->append(t); });
}

template<class T, class List>
auto SimpleListWidget<T, List>::setModel(Model *model) -> void
{
    if (m_model != model) {
        delete m_model;
        m_model = model;
        m_view->setModel(m_model);
        if (m_model)
            m_view->setHeaderHidden(m_model->header(0).isEmpty());
    }
}

class StringListWidget : public SimpleListWidget<QString, QStringList> {
    Q_OBJECT
public:
    StringListWidget(QWidget *parent = 0)
        : Super(parent) { setModel(new StringListModel(this)); }
private:
    auto getNewItem(QString *item) -> bool final;
};



#endif // SIMPLELISTWIDGET_HPP
