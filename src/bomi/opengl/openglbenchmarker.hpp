#ifndef OPENGLBENCHMARKER_HPP
#define OPENGLBENCHMARKER_HPP

#include <QOpenGLTimerQuery>

class OpenGLBenchmarker {
public:
    OpenGLBenchmarker() = default;
    OpenGLBenchmarker(bool init) { if (init) create(); }
    void create() {
        m_timer.create();
        m_ns = 0;
        m_counter = 0;
    }
    void destroy() { m_timer.destroy(); }
    void begin() { m_timer.begin(); }
    void end();
    void setMaxCount(int n) { m_max = n; }
private:
    QOpenGLTimerQuery m_timer;
    quint64 m_ns = 0;
    int m_counter = 0, m_max = 100;
};

#endif // OPENGLBENCHMARKER_HPP
