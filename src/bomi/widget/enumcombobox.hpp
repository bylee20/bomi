#ifndef ENUMCOMBOBOX_HPP
#define ENUMCOMBOBOX_HPP

#include "datacombobox.hpp"
#include "enum/generateplaylist.hpp"
#include "enum/autoloadmode.hpp"
#include "enum/autoselectmode.hpp"
#include "enum/clippingmethod.hpp"
#include "enum/interpolator.hpp"
#include "enum/textthemestyle.hpp"
#include "enum/channellayout.hpp"
#include "enum/verticalalignment.hpp"

template<class Enum>
class EnumComboBox : public DataComboBox {
public:
    EnumComboBox(QWidget *parent = 0): DataComboBox(parent) {
        setup();
    }
    EnumComboBox(bool autosetup, QWidget *parent = nullptr)
        : DataComboBox(parent) {
        if (autosetup)
            setup();
    }
    auto addValue(Enum e) -> void
        { addItem(EnumInfo<Enum>::description(e), QVariant::fromValue(e)); }
    auto value(int i) const -> Enum { return itemData(i).template value<Enum>(); }
    auto currentValue() const -> Enum { return currentData().template value<Enum>(); }
    auto setCurrentValue(Enum e) -> void { setCurrentData(QVariant::fromValue(e)); }
    auto setRetranslatable(bool retrans) -> void { m_retrans = retrans; }
private:
    auto setup() -> void {
        for (auto &item : EnumInfo<Enum>::items())
            addValue(item.value);
    }
    auto retranslate() -> void {
        for (int i = 0; i < count(); ++i) {
            auto e = itemData(i).template value<Enum>();
            setItemText(i, EnumInfo<Enum>::description(e));
        }
    }
    auto changeEvent(QEvent *event) -> void {
        QComboBox::changeEvent(event);
        if (event->type() == QEvent::LanguageChange && m_retrans)
            retranslate();
    }
    bool m_retrans = true;
};

using GeneratePlaylistComboBox = EnumComboBox<GeneratePlaylist>;
using AutoloadModeComboBox = EnumComboBox<AutoloadMode>;
using AutoselectModeComboBox = EnumComboBox<AutoselectMode>;
using ClippingMethodComboBox = EnumComboBox<ClippingMethod>;
using InterpolatorComboBox = EnumComboBox<Interpolator>;
using TextThemeStyleComboBox = EnumComboBox<TextThemeStyle>;
using ChannelComboBox = EnumComboBox<ChannelLayout>;
using VerticalAlignmentComboBox = EnumComboBox<VerticalAlignment>;

#endif // ENUMCOMBOBOX_HPP
