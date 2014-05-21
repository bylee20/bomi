#include "abrepeater.hpp"
#include "playengine.hpp"
#include "subtitle/subtitle.hpp"
#include "subtitle/subtitlerendereritem.hpp"

ABRepeater::ABRepeater(PlayEngine *engine, const SubtitleRendererItem *sub)
: QObject(engine) {
    m_engine = engine;
    m_sub = sub;
}

ABRepeater::~ABRepeater() {
    stop();
}

auto ABRepeater::onTick(int time) -> void
{
    if (m_repeating && time > m_b) {
        m_engine->seek(m_a);
        if (m_times < 0)
            emit repeated(-1);
        else {
            const int rest = m_times - (++m_nth);
            emit repeated(rest);
            if (rest <= 0) {
                stop();
                emit stopped();
            }
        }
    }
}

auto ABRepeater::setAToCurrentTime() -> int
{
    return (m_a = m_engine->time());
}

auto ABRepeater::setBToCurrentTime() -> int
{
    return (m_b = m_engine->time());
}

auto ABRepeater::setAToSubtitleTime() -> int
{
    return m_a = m_sub->start(m_engine->time());
}

auto ABRepeater::setBToSubtitleTime() -> int
{
    return m_b = m_sub->finish(m_engine->time());
}

auto ABRepeater::start(int times) -> bool
{
    if (m_repeating)
        stop();
    m_times = times;
    m_nth = 0;
    if ((m_repeating = (m_a >= 0 && m_b > m_a))) {
        connect(m_engine, SIGNAL(tick(int)), this, SLOT(onTick(int)));
        emit started();
    }
    return m_repeating;
}

auto ABRepeater::stop() -> void
{
    disconnect(m_engine, 0, this, 0);
    m_repeating = false;
    emit stopped();
}

