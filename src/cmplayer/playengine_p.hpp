#ifndef PLAYENGINE_P_HPP
#define PLAYENGINE_P_HPP

#include "info.hpp"
#include "videoframe.hpp"
#include "videooutput.hpp"
#include "hwacc.hpp"
#include "audiocontroller.hpp"
#include "playlistmodel.hpp"
#include "deintinfo.hpp"
#include "dataevent.hpp"
#include "playengine.hpp"
#include <mpvcore/mp_cmplayer.h>
#include <array>

extern "C" {
#include <video/decode/lavc.h>
#include <mpvcore/command.h>
#include <video/out/vo.h>
#include <video/decode/vd.h>
#include <mpvcore/playlist.h>
#include <mpvcore/codecs.h>
#include <mpvcore/m_property.h>
#include <mpvcore/input/input.h>
#include <audio/filter/af.h>
#include <video/filter/vf.h>
#include <audio/out/ao.h>
#include <stream/stream.h>
}
#undef min

enum EventType {
	UserType = QEvent::User, StreamOpen, UpdateTrack, StateChange, MrlStopped, MrlFinished, PlaylistFinished, MrlChanged, VideoFormatChanged, UpdateChapterList
};

enum MpCmd : int {
	MpSetProperty = std::numeric_limits<int>::min(),
	MpResetAudioChain, MpResetDeint, MpSetDeintEnabled, MpSetAudioLevel, MpSetAudioMuted
};

template<typename T> static QByteArray qbytearray_from(const T &t);
template<> inline QByteArray qbytearray_from(const QByteArray &t) { return t; }
template<> inline QByteArray qbytearray_from(const int &t) { return QByteArray::number(t); }
template<> inline QByteArray qbytearray_from(const float &t) { return QByteArray::number(t); }
template<> inline QByteArray qbytearray_from(const double &t) { return QByteArray::number(t); }
template<> inline QByteArray qbytearray_from(const QString &t) { return t.toLocal8Bit(); }

template<typename T> static inline T &getCmdArg(mp_cmd *cmd, int idx = 0) { static T t; (void)cmd->args[idx]; return t; }
template<> inline double&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.d;}
template<> inline float	&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.f;}
template<> inline int	&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.i;}
template<> inline char*	&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.s;}

extern void initialize_vdpau();
extern void finalize_vdpau();
extern void initialize_vaapi();
extern void finalize_vaapi();

void _ForVariadic() {}
template<typename Func, typename Arg1, typename... Args>
void _ForVariadic(Func func, const Arg1& arg1, const Args&... args) { func(arg1); _ForVariadic(args...); }

class PlayEngine::Thread : public QThread {
public:
	Thread(PlayEngine *engine): engine(engine) {}
private:
	PlayEngine *engine = nullptr;
	void run() { engine->exec(); }
};

class ImagePlayback {
public:
	int pos() const {return qBound(0, m_ticking ? m_pos + m_time.elapsed() : m_pos, m_duration);}
	void pause () { if (m_ticking) { m_pos += m_time.elapsed(); m_ticking = false; } }
	void moveBy(int diff) {
		m_pos += diff;
		if (m_ticking) {
			if (m_pos + m_time.elapsed() < 0)
				m_pos = -m_time.elapsed();
		} else {
			if (m_pos < 0)
				m_pos = 0;
		}
	}
	void start() { if (!m_ticking) { m_time.restart(); m_ticking = true; } }
	void restart() {
		m_pos = 0;
		m_time.restart();
		m_ticking = true;
	}
	bool isTicking() const { return m_ticking; }
	void setDuration(int duration) { m_duration = duration; }
	int duration() const { return m_duration; }
	void seek(int pos, bool relative) {
		if (m_duration > 0) {
			m_mutex.lock();
			m_by = pos;
			if (!relative)
				m_by -= this->pos();
			m_mutex.unlock();
		}
	}
	bool run(bool paused) {
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
