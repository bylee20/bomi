#include "simplelistwidget.hpp"

SimpleListWidgetBase::SimpleListWidgetBase(QWidget *parent)
    : QWidget(parent)
{
    m_view = new QTreeView(this);
    m_add = new QPushButton(tr("Add"), this);
    m_erase = new QPushButton(tr("Erase"), this);
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

auto StringListWidget::getNewItem(QString *item) -> bool
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Add"));
    QLineEdit *edit = new QLineEdit(&dlg);
    QPushButton *ok = new QPushButton(tr("&Ok"), &dlg);
    QPushButton *cancel = new QPushButton(tr("&Cancel"), &dlg);
    QHBoxLayout *hbox = new QHBoxLayout(&dlg);
    hbox->addWidget(edit);
    hbox->addWidget(ok);
    hbox->addWidget(cancel);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    if (!dlg.exec())
        return false;
    *item = edit->text();
    return true;
}
