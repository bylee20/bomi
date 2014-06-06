#include "openglbenchmarker.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(OpenGL)

void OpenGLBenchmarker::end()
{
    m_timer.end();
    m_ns += m_timer.waitForResult();
    ++m_counter;
    if (m_counter >= m_max) {
        _Info("%%op/s (%%ms for %% operations)",
              m_counter/(m_ns*1e-9),
              m_ns/1000000.0, m_counter);
        m_counter = 0;
        m_ns = 0;
    }
}
