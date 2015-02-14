#ifndef BBOX_HPP
#define BBOX_HPP

#include <QDialogButtonBox>

class BBox : public QDialogButtonBox {
    Q_OBJECT
public:
    using Role = QDialogButtonBox::ButtonRole;
    using Button = QDialogButtonBox::StandardButton;
    using Layout = QDialogButtonBox::ButtonLayout;
    static constexpr Layout SkinLayout = (Layout)-1;
    BBox(QWidget *parent = nullptr);
    auto setStandardButtons(StandardButtons buttons) -> void;
    auto addButton(StandardButton button) -> QPushButton*;
    auto addButton(const QString &text, Role role) -> QPushButton*
        { return QDialogButtonBox::addButton(text, role); }
    template<typename F>
    auto addButton(const QString &text, Role role, F func) -> QPushButton*
    {
        auto button = addButton(text, role);
        connect(button, &QPushButton::clicked, this, func);
        return button;
    }
    auto addButton(QAbstractButton *button, Role role) -> void
        { QDialogButtonBox::addButton(button, role); }
    static auto buttonText(Button button, Layout layout) -> QString;
    static auto buttonLayout(QWidget *w) -> Layout;
    static auto make(QDialog *dlg) -> BBox*;
private:
    Layout m_layout;
};

#endif // BBOX_HPP
