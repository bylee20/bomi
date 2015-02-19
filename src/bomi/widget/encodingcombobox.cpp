#include "encodingcombobox.hpp"
#include "misc/encodinginfo.hpp"

EncodingComboBox::EncodingComboBox(QWidget *parent)
: DataComboBox(parent) {
    for (auto &e : EncodingInfo::all())
        addItem(e.description(), e.mib());
    connect(SIGNAL_VT(this, currentIndexChanged, int),
            this, &EncodingComboBox::encodingChanged);
}

auto EncodingComboBox::encoding() const -> EncodingInfo
{
    return EncodingInfo::fromMib(currentValue<int>());
}

auto EncodingComboBox::setEncoding(const EncodingInfo &encoding) -> void
{
    setCurrentValue<int>(encoding.mib());
}
