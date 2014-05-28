#ifndef ABREPEATER_HPP
#define ABREPEATER_HPP

class PlayEngine;                       class SubtitleRendererItem;

class ABRepeater : public QObject {
    Q_OBJECT
public:
    ABRepeater(PlayEngine *engine, const SubtitleRendererItem *sub);
    ~ABRepeater();
    auto repeat(int a, int b, int times = -1) -> bool;
    auto isRepeating() -> bool {return m_repeating;}
    auto a() const -> int {return m_a;}
    auto b() const -> int {return m_b;}
    auto hasA() const -> bool {return m_a >= 0;}
    auto hasB() const -> bool {return m_b >= 0;}
    auto restTimes() const -> int {return m_times - m_nth;}
    auto times() const -> int {return m_times;}
    auto stop() -> void;
    auto start(int times = -1) -> bool;
    auto setAToCurrentTime() -> int;
    auto setBToCurrentTime() -> int;
    auto setAToSubtitleTime() -> int;
    auto setBToSubtitleTime() -> int;
    auto setA(int a) -> void {m_a = a;}
    auto setB(int b) -> void {m_b = b;}
signals:
    void repeated(int rest);
    void stopped();
    void started();
private:
    PlayEngine *m_engine = nullptr;
    const SubtitleRendererItem *m_sub = nullptr;
    int m_a = -1, m_b = -1;
    bool m_repeating = false;
    int m_times = 0, m_nth = 0;
};

inline auto ABRepeater::repeat(int a, int b, int times) -> bool
{m_a = a; m_b = b; return start(times);}

#endif // ABREPEATER_HPP
