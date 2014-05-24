#ifndef PLAYENGINE_P_HPP
#define PLAYENGINE_P_HPP

#include "playengine.hpp"

enum EventType {
    UserType = QEvent::User, UpdateTimeRange, UpdateTrackList, StateChange, PreparePlayback,
    UpdateChapterList, Tick, EndPlayback, StartPlayback,
    UpdateCache, UpdateCurrentStream, UpdateCurrentChapter,
    UpdateVideoInfo, UpdateAudioInfo, NotifySeek, UpdateMetaData
};

static inline auto qbytearray_from(const QByteArray &t) -> QByteArray { return t; }
static inline auto qbytearray_from(const bool &t) -> QByteArray { return QByteArray::number((int)t); }
static inline auto qbytearray_from(const int &t) -> QByteArray { return QByteArray::number(t); }
static inline auto qbytearray_from(const float &t) -> QByteArray { return QByteArray::number(t); }
static inline auto qbytearray_from(const double &t) -> QByteArray { return QByteArray::number(t); }
static inline auto qbytearray_from(const QString &t) -> QByteArray { return t.toLocal8Bit(); }
static inline auto qbytearray_from(const char *str) -> QByteArray { return QByteArray(str); }

extern auto initialize_vdpau() -> void;
extern auto finalize_vdpau() -> void;
extern auto initialize_vaapi() -> void;
extern auto finalize_vaapi() -> void;

class PlayEngine::Thread : public QThread {
public:
    Thread(PlayEngine *engine): engine(engine) {}
private:
    PlayEngine *engine = nullptr;
    auto run() -> void { engine->exec(); }
};

class ImagePlayback {
public:
    auto pos() const -> int {return qBound(0, m_ticking ? m_pos + m_time.elapsed() : m_pos, m_duration);}
    void pause () { if (m_ticking) { m_pos += m_time.elapsed(); m_ticking = false; } }
    auto moveBy(int diff) -> void {
        m_pos += diff;
        if (m_ticking) {
            if (m_pos + m_time.elapsed() < 0)
                m_pos = -m_time.elapsed();
        } else {
            if (m_pos < 0)
                m_pos = 0;
        }
    }
    auto start() -> void { if (!m_ticking) { m_time.restart(); m_ticking = true; } }
    auto restart() -> void {
        m_pos = 0;
        m_time.restart();
        m_ticking = true;
    }
    auto isTicking() const -> bool { return m_ticking; }
    auto setDuration(int duration) -> void { m_duration = duration; }
    auto duration() const -> int { return m_duration; }
    auto seek(int pos, bool relative) -> void {
        if (m_duration > 0) {
            m_mutex.lock();
            m_by = pos;
            if (!relative)
                m_by -= this->pos();
            m_mutex.unlock();
        }
    }
    auto run(bool paused) -> bool {
        m_mutex.lock();
        if (m_by)
            moveBy(m_by);
        m_by = 0;
        m_mutex.unlock();
        if (m_duration > 0) {
            if (paused)
                pause();
            else
                start();
            return pos() < m_duration;
        }
        return true;
    }
private:
    int m_duration = 1000, m_pos = 0, m_by = 0;
    bool m_ticking = true; QTime m_time; QMutex m_mutex;
};

#endif // PLAYENGINE_P_HPP
