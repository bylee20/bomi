#ifndef OPENMEDIABEHAVIORGROUPBOX_HPP
#define OPENMEDIABEHAVIORGROUPBOX_HPP

#include "player/openmediainfo.hpp"

class OpenMediaBehaviorGroupBox : public QGroupBox {
    Q_OBJECT
    Q_PROPERTY(OpenMediaInfo value READ value WRITE setValue NOTIFY valueChanged)
public:
    OpenMediaBehaviorGroupBox(QWidget *parent = nullptr);
    ~OpenMediaBehaviorGroupBox();
    auto setValue(const OpenMediaInfo &open) -> void;
    auto value() const -> OpenMediaInfo;
signals:
    void valueChanged();
private:
    struct Data;
    Data *d = nullptr;
};

#endif // OPENMEDIABEHAVIORGROUPBOX_HPP
