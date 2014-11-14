#ifndef SPEEDMEASURE_HPP
#define SPEEDMEASURE_HPP

#include "tmp/static_op.hpp"

template<class T>
class SpeedMeasure {
    struct Record {
        Record(quint64 time, const T &value)
            : value(value), usec(time) { }
        T value = 0;
        quint64 usec = 0;
    };
public:
    SpeedMeasure(int min, int max) { setDequeSize(min, max); }
    auto reset() -> void { m_records.clear(); }
    auto get() const -> double
        { return ((int)m_records.size() < m_min) ? 0.0 : dvalue()/dsec(); }
    auto push(const T &t) -> void
    {
        m_records.emplace_back(_SystemTime(), t);
        while ((int)m_records.size() > m_max)
            m_records.pop_front();
    }
    auto count() const -> int { return m_records.size(); }
    auto setDequeSize(int min, int max) -> void
    {
        Q_ASSERT(min > 1 && min <= max);
        m_min = min;
        m_max = max;
        while ((int)m_records.size() > m_max)
            m_records.pop_front();
    }
    auto dusec() const -> quint64
        { return m_records.back().usec - m_records.front().usec; }
    auto dsec() const -> double { return dusec() * 1e-6; }
    auto dvalue() const -> T
        { return m_records.back().value - m_records.front().value; }
private:
    std::deque<Record> m_records;
    int m_min = 2, m_max = 20;
};

#endif // SPEEDMEASURE_HPP
