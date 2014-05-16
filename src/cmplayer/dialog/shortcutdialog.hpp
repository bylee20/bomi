#ifndef SHORTCUTDIALOG_HPP
#define SHORTCUTDIALOG_HPP

#include "stdafx.hpp"

class ShortcutDialog : public QDialog {
    Q_OBJECT
public:
    ShortcutDialog(const QKeySequence &shortcut, QWidget *parent = 0);
    ~ShortcutDialog();
    auto shortcut() const -> QKeySequence;
    auto setShortcut(const QKeySequence &shortcut) -> void;
protected:
    auto eventFilter(QObject *obj, QEvent *event) -> bool;
    auto keyPressEvent(QKeyEvent *event) -> void;
    auto keyReleaseEvent(QKeyEvent *event) -> void;
private:
    struct Data;
    Data *d = nullptr;
};


#endif // SHORTCUTDIALOG_HPP
