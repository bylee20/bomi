#include "verticalalignment.hpp"

const std::array<VerticalAlignmentInfo::Item, 3> VerticalAlignmentInfo::info{{
    {VerticalAlignment::Top, "Top", "top", Qt::AlignTop},
    {VerticalAlignment::Center, "Center", "v-center", Qt::AlignVCenter},
    {VerticalAlignment::Bottom, "Bottom", "bottom", Qt::AlignBottom}
}};
