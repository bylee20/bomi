#ifndef MBOX_HPP
#define MBOX_HPP

#include "bbox.hpp"
#include <QMessageBox>

class MBox : public QObject {
    Q_DECLARE_TR_FUNCTIONS(MBox)
public:
    using Role = BBox::Role;
    using Button = BBox::Button;
    using Icon = QMessageBox::Icon;
    MBox(QWidget *parent = nullptr);
    MBox(QWidget *parent, Icon icon, const QString &title,
         const QString &text = QString(),
         std::initializer_list<Button> &&buttons = {},
         Button def = BBox::NoButton);
    ~MBox() { delete m_mbox; }
    auto addButton(const QString &text, Role role) -> QPushButton*;
    auto addButton(Button button) -> QPushButton*;
    auto addButtons(std::initializer_list<Button> &&buttons) -> void;
    auto exec() -> int { return m_mbox->exec(); }
    auto mbox() const -> QMessageBox* { return m_mbox; }
    auto checkBox() const -> QCheckBox*;
    auto isChecked() const -> bool;
    auto setInformativeText(const QString &text) -> void;
    auto setDetailedText(const QString &text) -> void;
    auto setDefaultButton(Button button) -> void;
    auto setIcon(Icon icon) -> void { m_mbox->setIcon(icon); }
    auto setTitle(const QString &title) -> void;
    auto setText(const QString &text) -> void { m_mbox->setText(text); }
    auto role(QAbstractButton *button) const -> Role;
    auto clickedRole() const -> Role { return role(m_mbox->clickedButton()); }
#define DEC_POPUP(func, icon) \
    static auto func(QWidget *parent, const QString &title, \
                     const QString &text, \
                     std::initializer_list<Button> &&buttons,\
                     Button def = BBox::NoButton) -> int \
    { \
        MBox mbox(parent, icon, title, text, \
                  std::forward<std::initializer_list<Button>>(buttons), def); \
        return mbox.exec();\
    }
    DEC_POPUP(warn, Icon::Warning)
    DEC_POPUP(info, Icon::Information)
    DEC_POPUP(error, Icon::Critical)
    DEC_POPUP(ask, Icon::Question)
#undef DEC_POPUP
private:
    QMessageBox *m_mbox = nullptr;
    BBox::Layout m_layout;
};

inline auto MBox::addButton(const QString &text, Role role) -> QPushButton*
{ return m_mbox->addButton(text, (QMessageBox::ButtonRole)role); }

inline auto MBox::addButton(Button button) -> QPushButton*
{
    const auto b = static_cast<QMessageBox::StandardButton>(button);
    const auto ret = m_mbox->addButton(b);
    ret->setText(BBox::buttonText(button, m_layout));
    return ret;
}

inline auto MBox::checkBox() const -> QCheckBox*
{
    if (!m_mbox->checkBox())
        m_mbox->setCheckBox(new QCheckBox);
    return m_mbox->checkBox();
}

inline auto MBox::isChecked() const -> bool
{ return m_mbox->checkBox() && m_mbox->checkBox()->isCheckable(); }

inline auto MBox::setInformativeText(const QString &text) -> void
{ m_mbox->setInformativeText(text); }

inline auto MBox::setDetailedText(const QString &text) -> void
{ m_mbox->setDetailedText(text); }

inline auto MBox::setDefaultButton(Button button) -> void
{ m_mbox->setDefaultButton((QMessageBox::StandardButton)button); }

inline auto MBox::setTitle(const QString &title) -> void
{ _SetWindowTitle(m_mbox, title); }

inline auto MBox::role(QAbstractButton *button) const -> Role
{ return static_cast<Role>(m_mbox->buttonRole(button)); }

#endif // MBOX_HPP
