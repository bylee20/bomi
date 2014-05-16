#include "mbox.hpp"

MBox::MBox(QWidget *parent)
    : QObject(parent)
{
    m_mbox = new QMessageBox(parent);
    m_layout = BBox::buttonLayout(m_mbox);
}

MBox::MBox(QWidget *parent, Icon icon, const QString &title,
           const QString &text, std::initializer_list<Button> &&buttons,
           Button def)
    : MBox(parent)
{
    addButtons(std::forward<std::initializer_list<Button>>(buttons));
    setTitle(title);
    setText(text);
    setDefaultButton(def);
    setIcon(icon);
}

auto MBox::addButtons(std::initializer_list<Button> &&buttons) -> void
{
    for (auto b : buttons)
        addButton(b);
}
