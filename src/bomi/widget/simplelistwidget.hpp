#ifndef SIMPLELISTWIDGET_HPP
#define SIMPLELISTWIDGET_HPP

#include "misc/simplelistmodel.hpp"
#include <QTreeView>

class SimpleListWidgetBase : public QWidget {
    Q_OBJECT
public:
    SimpleListWidgetBase(QWidget *parent = nullptr);
    auto setAddingAndErasingEnabled(bool enabled) -> void;
    auto setChangingOrderEnabled(bool enabled) -> void;
    auto isChangingOrderEnabled() const -> bool { return m_movable; }
    auto isAddingAndErasingEnabled() const -> bool { return m_addable; }
    auto view() const -> QTreeView* { return m_view; }
    auto setDragEnabled(bool enabled) -> void;
protected:
    auto setModel(SimpleListModelBase *base) -> void;
    virtual auto append() -> void = 0;
private:
    QTreeView *m_view;
    QPushButton *m_add, *m_erase, *m_up, *m_down;
    bool m_addable = false, m_movable = false;
    SimpleListModelBase *m_base = nullptr;
};

template<class M>
class SimpleListWidget : public SimpleListWidgetBase {
    using List = typename M::container_type;
    using T = typename M::value_type;
public:
    using Super = SimpleListWidget<M>;
    using Model = M;
    SimpleListWidget(QWidget *parent = 0)
        : SimpleListWidgetBase(parent) { setModel(new M(this)); }
    auto setList(const List &list) -> void { model()->setList(list); }
    auto list() const -> List { return model()->list(); }
    auto model() const -> M* { return static_cast<M*>(view()->model()); }
protected:
    virtual auto getNewItem(T *item) -> bool { Q_UNUSED(item); return false; }
    auto append() -> void final { T t; if (model() && getNewItem(&t)) model()->append(t); }
};

class StringListWidget : public SimpleListWidget<StringListModel> {
    Q_OBJECT
    Q_PROPERTY(QStringList value READ list WRITE setList NOTIFY contentsChanged)
public:
    StringListWidget(QWidget *parent = 0)
        : Super(parent)
    {
        connect(model(), &SimpleListModelBase::contentsChanged,
                this, &StringListWidget::contentsChanged);
    }
signals:
    void contentsChanged();
private:
    auto getNewItem(QString *item) -> bool final;
};



#endif // SIMPLELISTWIDGET_HPP
