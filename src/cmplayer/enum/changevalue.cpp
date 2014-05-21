#include "changevalue.hpp"

const std::array<ChangeValueInfo::Item, 3> ChangeValueInfo::info{{
    {ChangeValue::Reset, "Reset", "reset", 0},
    {ChangeValue::Increase, "Increase", "increase", 1},
    {ChangeValue::Decrease, "Decrease", "decrease", -1}
}};
