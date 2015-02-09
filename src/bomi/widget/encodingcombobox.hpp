#ifndef ENCODINGCOMBOBOX_HPP
#define ENCODINGCOMBOBOX_HPP

#include "datacombobox.hpp"

class EncodingComboBox : public DataComboBox {
    Q_OBJECT
    Q_PROPERTY(QString encoding READ encoding WRITE setEncoding NOTIFY encodingChanged)
public:
    EncodingComboBox(QWidget *parent = 0);
    auto encoding() const -> QString;
    auto setEncoding(const QString &encoding) -> void;
signals:
    void encodingChanged();
};

#endif // ENCODINGCOMBOBOX_HPP
