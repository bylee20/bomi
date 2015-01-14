#ifndef ABREPEATCHECKER_HPP
#define ABREPEATCHECKER_HPP

class ABRepeatChecker {
public:
    ABRepeatChecker() noexcept { }
    ~ABRepeatChecker() noexcept { stop(); }
    auto repeat(int a, int b, int times = -1) -> bool
        { m_a = a; m_b = b; return start(times); }
    auto isRepeating() -> bool { return m_repeating; }
    auto a() const -> int { return m_a; }
    auto b() const -> int { return m_b; }
    auto hasA() const -> bool { return m_a >= 0; }
    auto hasB() const -> bool { return m_b >= 0; }
    auto restTimes() const -> int { return m_times - m_nth; }
    auto times() const -> int { return m_times; }
    auto stop() -> void { m_repeating = false; }
    auto setA(int a) -> int { return m_a = a; }
    auto setB(int b) -> int { return m_b = b; }
    auto check(int time) -> bool;
    auto start(int times = -1) -> bool;
private:
    int m_a = -1, m_b = -1;
    bool m_repeating = false;
    int m_times = 0, m_nth = 0;
};

#endif // ABREPEATCHECKER_HPP
