#ifndef MOUSEACTIONGROUPBOX_HPP
#define MOUSEACTIONGROUPBOX_HPP

struct KeyModifierActionMap;

class MouseActionGroupBox : public QGroupBox {
    Q_OBJECT
public:
    using Action = QPair<QString, QString>;
    MouseActionGroupBox(QWidget *parent = nullptr);
    ~MouseActionGroupBox();
    auto set(const QList<QPair<QString, QString>> &list) -> void;
    auto setValues(const KeyModifierActionMap &map) -> void;
    auto values() const -> KeyModifierActionMap;
private:
    struct Data;
    Data *d;
};

#endif // MOUSEACTIONGROUPBOX_HPP
