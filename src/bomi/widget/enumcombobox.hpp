#ifndef ENUMCOMBOBOX_HPP
#define ENUMCOMBOBOX_HPP

#include "datacombobox.hpp"
#include "enum/generateplaylist.hpp"
#include "enum/autoloadmode.hpp"
#include "enum/autoselectmode.hpp"
#include "enum/interpolator.hpp"
#include "enum/textthemestyle.hpp"
#include "enum/channellayout.hpp"
#include "enum/verticalalignment.hpp"
#include "enum/jrconnection.hpp"
#include "enum/jrprotocol.hpp"

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
    auto addEnum(Enum e) -> void
        { addItem<Enum>(EnumInfo<Enum>::description(e), e); }
    auto enum_(int i) const -> Enum { return value<Enum>(i); }
    auto currentEnum() const -> Enum { return currentValue<Enum>(); }
    auto setCurrentEnum(Enum e) -> void { setCurrentValue(e); }
    auto setRetranslatable(bool retrans) -> void { m_retrans = retrans; }
private:
    auto setup() -> void {
        for (auto &item : EnumInfo<Enum>::items())
            addEnum(item.value);
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
using InterpolatorComboBox = EnumComboBox<Interpolator>;
using TextThemeStyleComboBox = EnumComboBox<TextThemeStyle>;
using ChannelComboBox = EnumComboBox<ChannelLayout>;
using VerticalAlignmentComboBox = EnumComboBox<VerticalAlignment>;
using JrConnectionComboBox = EnumComboBox<JrConnection>;
using JrProtocolComboBox = EnumComboBox<JrProtocol>;

#endif // ENUMCOMBOBOX_HPP
