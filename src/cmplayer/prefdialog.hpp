#ifndef PREFDIALOG_HPP
#define PREFDIALOG_HPP

#include "stdafx.hpp"

class QTreeWidgetItem;        class QAbstractButton;
class Pref;

class PrefDialog : public QDialog {
    Q_OBJECT
public:
    PrefDialog(QWidget *parent = 0);
    ~PrefDialog();
    auto set(const Pref &pref) -> void;
    auto get(Pref &p) -> void;
signals:
    void applyRequested();
    void resetRequested();
private:
    void changeEvent(QEvent *event);
    void showEvent(QShowEvent *event);
    class MenuTreeItem;
    class Delegate;
    struct Data;
    Data *d;
};

#endif // PREFDIALOG_HPP
