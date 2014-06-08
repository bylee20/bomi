#include "deintmode.hpp"

const std::array<DeintModeInfo::Item, 2> DeintModeInfo::info{{
    {DeintMode::None, u"None"_q, u"off"_q, (int)0},
    {DeintMode::Auto, u"Auto"_q, u"auto"_q, (int)1}
}};
