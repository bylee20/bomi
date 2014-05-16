#include "datacombobox.hpp"

DataComboBox::DataComboBox(QWidget *parent)
    : QComboBox(parent)
{
    Signal<QComboBox, int> changed = &QComboBox::currentIndexChanged;
    connect(this, changed, [this] (int idx) {
        emit currentDataChanged(idx < 0 ? QVariant() : itemData(idx));
    });
}
