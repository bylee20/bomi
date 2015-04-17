#include "visualization.hpp"

const std::array<VisualizationInfo::Item, 2> VisualizationInfo::info{{
    {Visualization::Off, u"Off"_q, u"off"_q, (int)0},
    {Visualization::Bar, u"Bar"_q, u"bar"_q, (int)1}
}};
