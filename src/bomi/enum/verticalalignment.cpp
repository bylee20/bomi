#include "verticalalignment.hpp"

const std::array<VerticalAlignmentInfo::Item, 3> VerticalAlignmentInfo::info{{
    {VerticalAlignment::Top, u"Top"_q, u"top"_q, Qt::AlignTop},
    {VerticalAlignment::Center, u"Center"_q, u"v-center"_q, Qt::AlignVCenter},
    {VerticalAlignment::Bottom, u"Bottom"_q, u"bottom"_q, Qt::AlignBottom}
}};
