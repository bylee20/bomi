#ifndef OPENMEDIABEHAVIORGROUPBOX_HPP
#define OPENMEDIABEHAVIORGROUPBOX_HPP

struct OpenMediaInfo;

class OpenMediaBehaviorGroupBox : public QGroupBox {
    Q_OBJECT
public:
    OpenMediaBehaviorGroupBox(QWidget *parent = nullptr);
    ~OpenMediaBehaviorGroupBox();
    auto setValue(const OpenMediaInfo &open) -> void;
    auto value() const -> OpenMediaInfo;
private:
    struct Data;
    Data *d = nullptr;
};

#endif // OPENMEDIABEHAVIORGROUPBOX_HPP
