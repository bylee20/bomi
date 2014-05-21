#include "keymodifier.hpp"

const std::array<KeyModifierInfo::Item, 4> KeyModifierInfo::info{{
    {KeyModifier::None, "None", "", (int)Qt::NoModifier},
    {KeyModifier::Ctrl, "Ctrl", "", (int)Qt::ControlModifier},
    {KeyModifier::Shift, "Shift", "", (int)Qt::ShiftModifier},
    {KeyModifier::Alt, "Alt", "", (int)Qt::AltModifier}
}};
