#include "abrepeatchecker.hpp"

auto ABRepeatChecker::check(int time) -> bool
{
    if (!m_repeating || time <= m_b)
        return false;
    if (m_times < 0)
        return true;
    if (m_times <= ++m_nth)
        stop();
    return true;
}

auto ABRepeatChecker::start(int times) -> bool
{
    if (m_repeating)
        stop();
    m_times = times;
    m_nth = 0;
    m_repeating = (m_a >= 0 && m_b > m_a);
    return m_repeating;
}
