#ifndef SHORTCUTDIALOG_HPP
#define SHORTCUTDIALOG_HPP

#include <functional>

class ShortcutDialog : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(ShortcutDialog)
public:
    using QueryFunc = std::function<QString(const QString&, const QKeySequence&)>;
    ShortcutDialog(const QKeySequence &shortcut, QWidget *parent = 0);
    ~ShortcutDialog();
    auto shortcut() const -> QKeySequence;
    auto setQueryFunction(const QueryFunc &func, const QString &id) -> void;
protected:
    auto eventFilter(QObject *obj, QEvent *event) -> bool;
    auto keyPressEvent(QKeyEvent *event) -> void;
    auto keyReleaseEvent(QKeyEvent *event) -> void;
private:
    auto setShortcut(const QKeySequence &shortcut) -> void;
    struct Data;
    Data *d = nullptr;
};


#endif // SHORTCUTDIALOG_HPP
