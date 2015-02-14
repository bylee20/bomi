#ifndef SPEEDMEASURE_HPP
#define SPEEDMEASURE_HPP

#include <QElapsedTimer>
#include <functional>
#include <deque>

template<class T>
class SpeedMeasure {
    struct Record {
        Record(quint64 time, const T &value)
            : value(value), usec(time) { }
        T value = 0;
        quint64 usec = 0;
    };
public:
    SpeedMeasure(int min, int max)
    {
        setDequeSize(min, max);
        m_watch.start();
    }
    auto reset() -> void { m_records.clear(); m_last = 0; }
    auto get() const -> double
        { return ((int)m_records.size() < m_min) ? 0.0 : dvalue()/dsec(); }
    auto push(const T &t) -> void
    {
        const quint64 usec = m_watch.nsecsElapsed() * 1e-3;
        m_records.emplace_back(usec, t);
        while ((int)m_records.size() > m_max)
            m_records.pop_front();
        if (m_interval > 0 && m_timer) {
            if (!m_last || m_last > usec ) {
                m_last = usec;
            } else if (usec - m_last > m_interval) {
                m_timer();
                m_last = usec;
            }
        }
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
    auto setTimer(std::function<void(void)> &&timer,
                  quint64 usec = 5000000) -> void
        { m_timer = std::move(timer); m_interval = usec; }
private:
    std::deque<Record> m_records;
    int m_min = 2, m_max = 20;
    quint64 m_last = 0, m_interval = 0;
    std::function<void(void)> m_timer;
    QElapsedTimer m_watch;
};

#endif // SPEEDMEASURE_HPP
