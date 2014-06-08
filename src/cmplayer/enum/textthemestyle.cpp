#include "textthemestyle.hpp"

const std::array<TextThemeStyleInfo::Item, 4> TextThemeStyleInfo::info{{
    {TextThemeStyle::Normal, u"Normal"_q, u""_q, (int)0},
    {TextThemeStyle::Outline, u"Outline"_q, u""_q, (int)1},
    {TextThemeStyle::Raised, u"Raised"_q, u""_q, (int)2},
    {TextThemeStyle::Sunken, u"Sunken"_q, u""_q, (int)3}
}};
