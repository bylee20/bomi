#ifndef PREFDIALOG_HPP
#define PREFDIALOG_HPP

#include "stdafx.hpp"

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
    auto changeEvent(QEvent *event) -> void;
    auto showEvent(QShowEvent *event) -> void;
    class MenuTreeItem;
    class Delegate;
    struct Data;
    Data *d;
};

#endif // PREFDIALOG_HPP
