#include "simplelistwidget.hpp"
#include "dialog/bbox.hpp"

SimpleListWidgetBase::SimpleListWidgetBase(QWidget *parent)
    : QWidget(parent)
{
    m_view = new QTreeView(this);
    m_add = new QPushButton(tr("Add"), this);
    m_erase = new QPushButton(tr("Remove"), this);
    m_up = new QPushButton(tr("Move Up"), this);
    m_down = new QPushButton(tr("Move Down"), this);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(m_add);
    hbox->addWidget(m_erase);
    hbox->addWidget(m_up);
    hbox->addWidget(m_down);
    vbox->addLayout(hbox);
    vbox->addWidget(m_view);
    vbox->setMargin(0);
    hbox->setMargin(0);

    setAddingAndErasingEnabled(false);
    setChangingOrderEnabled(false);

    m_view->setDragDropMode(QAbstractItemView::InternalMove);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setDragDropOverwriteMode(false);
    m_view->setRootIsDecorated(false);

    connect(m_erase, &QPushButton::clicked, this, [=] () {
        const int row = m_view->currentIndex().row();
        if (!m_base || row < 0)
            return;
        m_base->remove(row);
        m_view->setCurrentIndex(m_base->index(row));
    });
    auto move = [this] (int diff)
    {
        if (m_base) {
            const int row = m_view->currentIndex().row();
            if (m_base->swap(row, row + diff))
                m_view->setCurrentIndex(m_base->index(row + diff));
        }
    };
    connect(m_up, &QPushButton::clicked, this, [=] () { move(-1); });
    connect(m_down, &QPushButton::clicked, this, [=] () { move(1); });
    connect(m_add, &QPushButton::clicked, this, &SimpleListWidgetBase::append);
}

auto SimpleListWidgetBase::setModel(SimpleListModelBase *base) -> void
{
    Q_ASSERT(!m_base);
    m_base = base;
    m_view->setModel(m_base);
    m_view->setHeaderHidden(m_base->header(0).isEmpty());

    auto sm = m_view->selectionModel();
    connect(sm, &QItemSelectionModel::selectionChanged,
            this, [=] (const QItemSelection &selected)
    {
        int min = m_base->rows(), max = -1;
        for (auto idx : selected.indexes()) {
            if (idx.row() < min)
                min = idx.row();
            if (idx.row() > max)
                max = idx.row();
        }
        m_erase->setEnabled(m_addable && max != -1);
        m_up->setEnabled(_InRange(1, min, m_base->rows() - 1));
        m_down->setEnabled(_InRange(0, max, m_base->rows() - 2));
    });
}

auto SimpleListWidgetBase::setAddingAndErasingEnabled(bool enabled) -> void
{
    m_addable = enabled;
    m_add->setEnabled(m_addable);
    m_erase->setEnabled(m_addable);
    m_add->setVisible(m_addable);
    m_erase->setVisible(m_addable);
}

auto SimpleListWidgetBase::setChangingOrderEnabled(bool enabled) -> void
{
    m_movable = enabled;
    m_up->setEnabled(m_movable);
    m_down->setEnabled(m_movable);
    m_up->setVisible(m_movable);
    m_down->setVisible(m_movable);
}

auto SimpleListWidgetBase::setDragEnabled(bool enabled) -> void
{
    m_view->setDragEnabled(enabled);
    m_view->viewport()->setAcceptDrops(enabled);
    m_view->setDropIndicatorShown(enabled);
}

auto StringListWidget::getNewItem(QString *item) -> bool
{
    QDialog dlg(this);
    _SetWindowTitle(&dlg, tr("Add"));
    auto edit = new QLineEdit(&dlg);
    auto hbox = new QHBoxLayout(&dlg);
    hbox->addWidget(edit);
    hbox->addWidget(BBox::make(&dlg));
    if (!dlg.exec())
        return false;
    *item = edit->text();
    return true;
}
