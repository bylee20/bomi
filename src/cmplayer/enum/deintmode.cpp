#include "deintmode.hpp"

const std::array<DeintModeInfo::Item, 2> DeintModeInfo::info{{
    {DeintMode::None, "None", "off", (int)0},
    {DeintMode::Auto, "Auto", "auto", (int)1}
}};
