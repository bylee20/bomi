#include "changevalue.hpp"

const std::array<ChangeValueInfo::Item, 3> ChangeValueInfo::info{{
    {ChangeValue::Reset, u"Reset"_q, u"reset"_q, 0},
    {ChangeValue::Increase, u"Increase"_q, u"increase"_q, 1},
    {ChangeValue::Decrease, u"Decrease"_q, u"decrease"_q, -1}
}};
