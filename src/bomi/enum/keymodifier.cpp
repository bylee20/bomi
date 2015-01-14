#include "keymodifier.hpp"

const std::array<KeyModifierInfo::Item, 4> KeyModifierInfo::info{{
    {KeyModifier::None, u"None"_q, u""_q, (int)Qt::NoModifier},
    {KeyModifier::Ctrl, u"Ctrl"_q, u""_q, (int)Qt::ControlModifier},
    {KeyModifier::Shift, u"Shift"_q, u""_q, (int)Qt::ShiftModifier},
    {KeyModifier::Alt, u"Alt"_q, u""_q, (int)Qt::AltModifier}
}};
