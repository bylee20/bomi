#include "horizontalalignment.hpp"

const std::array<HorizontalAlignmentInfo::Item, 3> HorizontalAlignmentInfo::info{{
    {HorizontalAlignment::Left, u"Left"_q, u"left"_q, Qt::AlignLeft},
    {HorizontalAlignment::Center, u"Center"_q, u"h-center"_q, Qt::AlignHCenter},
    {HorizontalAlignment::Right, u"Right"_q, u"right"_q, Qt::AlignRight}
}};
