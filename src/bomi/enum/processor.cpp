#include "processor.hpp"

const std::array<ProcessorInfo::Item, 3> ProcessorInfo::info{{
    {Processor::None, u"None"_q, u""_q, (int)0},
    {Processor::CPU, u"CPU"_q, u""_q, (int)1},
    {Processor::GPU, u"GPU"_q, u""_q, (int)2}
}};
