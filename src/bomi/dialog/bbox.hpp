#ifndef BBOX_HPP
#define BBOX_HPP

#include <QDialogButtonBox>

class BBox : public QDialogButtonBox {
    Q_DECLARE_TR_FUNCTIONS(BBox)
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
        { return addButton<F>(text, role, this, func); }
    template<typename F>
    auto addButton(const QString &text, Role role, QObject *ctx, F func) -> QPushButton*
    {
        auto button = addButton(text, role);
        connect(button, &QPushButton::clicked, ctx, func);
        return button;
    }
    auto addButton(QAbstractButton *button, Role role) -> void
        { QDialogButtonBox::addButton(button, role); }
    static auto buttonText(Button button, Layout layout) -> QString;
    static auto buttonText(Button button) -> QString;
    static auto buttonLayout(QWidget *w) -> Layout;
    static auto make(QDialog *dlg) -> BBox*;
private:
    Layout m_layout;
};

#endif // BBOX_HPP
