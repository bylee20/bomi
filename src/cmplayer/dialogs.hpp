#ifndef DIALOGS_HPP
#define DIALOGS_HPP

#include "stdafx.hpp"

class BBox : public QDialogButtonBox {
    Q_OBJECT
public:
    using Role = QDialogButtonBox::ButtonRole;
    using Button = QDialogButtonBox::StandardButton;
    using Layout = QDialogButtonBox::ButtonLayout;
    static constexpr Layout SkinLayout = (Layout)-1;
    BBox(QWidget *parent = nullptr);
    auto setStandardButtons(StandardButtons buttons) -> void;
    auto addButton(StandardButton button) -> void;
    static auto buttonText(Button button, Layout layout) -> QString;
    static auto buttonLayout(QWidget *w) -> Layout;
    static auto make(QDialog *dlg) -> BBox*;
private:
    Layout m_layout;
};

/******************************************************************************/

class MBox : public QObject {
    Q_OBJECT
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
    auto addButton(const QString &text, Role role) -> void;
    auto addButton(Button button) -> void;
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
#undef DEC_POPUP
private:
    QMessageBox *m_mbox = nullptr;
    BBox::Layout m_layout;
};

inline auto MBox::addButton(const QString &text, Role role) -> void
{ m_mbox->addButton(text, (QMessageBox::ButtonRole)role); }

inline auto MBox::addButton(Button button) -> void
{
    const auto b = static_cast<QMessageBox::StandardButton>(button);
    m_mbox->addButton(b)->setText(BBox::buttonText(button, m_layout));
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
{ m_mbox->setWindowTitle(title); }

inline auto MBox::role(QAbstractButton *button) const -> Role
{ return static_cast<Role>(m_mbox->buttonRole(button)); }

/******************************************************************************/

class GetShortcutDialog : public QDialog {
    Q_OBJECT
public:
    GetShortcutDialog(const QKeySequence &shortcut, QWidget *parent = 0);
    ~GetShortcutDialog();
    auto shortcut() const -> QKeySequence;
    auto setShortcut(const QKeySequence &shortcut) -> void;
protected:
    auto eventFilter(QObject *obj, QEvent *event) -> bool;
    auto keyPressEvent(QKeyEvent *event) -> void;
    auto keyReleaseEvent(QKeyEvent *event) -> void;
private:
    auto setGetting(bool on) -> void;
    auto erase() -> void;
    auto getShortcut(QKeyEvent *event) -> void;
    static const int MaxKeyCount = 1;
    struct Data;
    Data *d = nullptr;
};

class EncodingComboBox;

class EncodingFileDialog : public QFileDialog {
    Q_OBJECT
public:
    static auto getOpenFileName(QWidget *parent = 0,
                                const QString &caption = QString(),
                                const QString &dir = QString(),
                                const QString &filter = QString(),
                                QString *enc = 0) -> QString;
    static auto getOpenFileNames(QWidget *parent = 0,
                                 const QString &caption = QString(),
                                 const QString &dir = QString(),
                                 const QString &filter = QString(),
                                 QString *enc = 0,
                                 FileMode = ExistingFiles) -> QStringList;
private:
    EncodingFileDialog(QWidget *parent = 0,
                       const QString &caption = QString(),
                       const QString &directory = QString(),
                       const QString &filter = QString(),
                       const QString &encoding = QString());
    auto setEncoding(const QString &encoding) -> void;
    auto encoding() const -> QString;
    EncodingComboBox *combo = nullptr;
};

class Playlist;

class GetUrlDialog : public QDialog {
    Q_OBJECT
public:
    GetUrlDialog(QWidget *parent = 0);
    ~GetUrlDialog();
    auto setUrl(const QUrl &url) -> void;
    auto url() const -> QUrl;
    auto isPlaylist() const -> bool;
    auto playlist() const -> Playlist;
    auto encoding() const -> QString;
private:
    auto accept() -> void;
    auto _accept() -> void;
    struct Data;
    Data *d;
};

class AboutDialog : public QDialog {
    Q_OBJECT
public:
    AboutDialog(QWidget *parent = 0);
    ~AboutDialog();
private:
    auto showFullLicense() -> void;
    struct Data;
    Data *d = nullptr;
};

class OpenDiscDialog : public QDialog {
    Q_OBJECT
public:
    OpenDiscDialog(QWidget *parent = 0);
    ~OpenDiscDialog();
    auto setDeviceList(const QStringList &devices) -> void;
    auto setDevice(const QString &device) -> void;
    auto setIsoEnabled(bool on) -> void;
    auto device() const -> QString;
    auto checkDevice(const QString &device) -> void;
private:
    struct Data;
    Data *d = nullptr;
};


#endif // DIALOGS_HPP
