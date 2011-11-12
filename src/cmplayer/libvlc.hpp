#ifndef LIBVLC_H
#define LIBVLC_H

#include <vlc/vlc.h>
#include <QtCore/QString>
#include "mrl.hpp"

class AudioController;		class AudioBuffer;
class VideoRenderer;		class PlayEngine;
class AudioFormat;		class VideoFormat;

class LibVLC {
public:
	static void initialize();
	static void finalize();
	static PlayEngine *engine() {Q_ASSERT(d != 0 && m_engine != 0); return m_engine;}
	static AudioController *audio() {Q_ASSERT(d != 0 && m_audio != 0); return m_audio;}
	static VideoRenderer *video() {Q_ASSERT(d != 0 && m_video != 0); return m_video;}
	static libvlc_instance_t *inst() {Q_ASSERT(d != 0 && m_inst != 0); return m_inst;}
	static libvlc_media_player_t *mp() {Q_ASSERT(d != 0 && m_mp != 0); return m_mp;}
	static libvlc_media_t *newMedia(const Mrl &mrl) {Q_ASSERT(d && m_inst);
		return libvlc_media_new_location(m_inst, mrl.toString().toLocal8Bit());
	}
	static void outputError() {qWarning("LibVLC error: %s", libvlc_errmsg());}
private:
	static void cbAudioPrepare(void *data, const AudioFormat *format);
	static AudioBuffer *cbAudioProcess(void *data, AudioBuffer *in);
	static void *cbVideoLock(void *data, void ***planes);
	static void cbVideoRender(void *data, void **planes);
	static void cbVideoProcess(void *data, void **planes);
	static void cbVideoUnlock(void *data, void *id, void *const *plane);
	static void cbVideoDisplay(void *data, void *id);
	static void cbVideoPrepare(void *data, const VideoFormat *format);
	static void cbManageEvent(const libvlc_event_t *event, void *data);
	LibVLC();
	struct Data;
	static Data *d;

	static PlayEngine *m_engine;
	static AudioController *m_audio;
	static VideoRenderer *m_video;
	static libvlc_instance_t *m_inst;
	static libvlc_media_player_t *m_mp;
};

#endif // LIBVLC_H
