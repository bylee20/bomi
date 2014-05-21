#include "horizontalalignment.hpp"

const std::array<HorizontalAlignmentInfo::Item, 3> HorizontalAlignmentInfo::info{{
    {HorizontalAlignment::Left, "Left", "left", Qt::AlignLeft},
    {HorizontalAlignment::Center, "Center", "h-center", Qt::AlignHCenter},
    {HorizontalAlignment::Right, "Right", "right", Qt::AlignRight}
}};
