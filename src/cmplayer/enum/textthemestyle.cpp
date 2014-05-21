#include "textthemestyle.hpp"

const std::array<TextThemeStyleInfo::Item, 4> TextThemeStyleInfo::info{{
    {TextThemeStyle::Normal, "Normal", "", (int)0},
    {TextThemeStyle::Outline, "Outline", "", (int)1},
    {TextThemeStyle::Raised, "Raised", "", (int)2},
    {TextThemeStyle::Sunken, "Sunken", "", (int)3}
}};
