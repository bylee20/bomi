#ifndef ENCODINGCOMBOBOX_HPP
#define ENCODINGCOMBOBOX_HPP

#include "datacombobox.hpp"
#include "misc/encodinginfo.hpp"

class EncodingComboBox : public DataComboBox {
    Q_OBJECT
    Q_PROPERTY(EncodingInfo encoding READ encoding WRITE setEncoding NOTIFY encodingChanged)
public:
    EncodingComboBox(QWidget *parent = 0);
    auto encoding() const -> EncodingInfo;
    auto setEncoding(const EncodingInfo &encoding) -> void;
signals:
    void encodingChanged();
};

#endif // ENCODINGCOMBOBOX_HPP
