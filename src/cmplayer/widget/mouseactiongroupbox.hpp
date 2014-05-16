#ifndef MOUSEACTIONGROUPBOX_HPP
#define MOUSEACTIONGROUPBOX_HPP

#include "stdafx.hpp"

struct KeyModifierActionMap;

class MouseActionGroupBox : public QGroupBox {
    Q_OBJECT
public:
    struct Action { QString name, id; };
    MouseActionGroupBox(QWidget *parent = nullptr);
    ~MouseActionGroupBox();
    auto set(const QList<Action> &list) -> void;
    auto setValues(const KeyModifierActionMap &map) -> void;
    auto values() const -> KeyModifierActionMap;
private:
    struct Data;
    Data *d;
};


#endif // MOUSEACTIONGROUPBOX_HPP
