#include "audiocontroller.hpp"
#include "info.hpp"
#include "videorenderer.hpp"
#include "videoframe.hpp"
#include "playengine.hpp"
#include "libvlc.hpp"
#include <QtCore/QDebug>

struct LibVLC::Data {};

LibVLC::Data *LibVLC::d = 0;
PlayEngine *LibVLC::m_engine = 0;
AudioController *LibVLC::m_audio = 0;
VideoRenderer *LibVLC::m_video = 0;
libvlc_instance_t *LibVLC::m_inst = 0;
libvlc_media_player_t *LibVLC::m_mp = 0;

void LibVLC::initialize() {
	Q_ASSERT(d == 0);
	d = new Data;

	static VideoUtil util_v;
	static AudioUtil util_a;

	util_v.vd = util_a.af = 0;
	util_a.scaletempo_enabled = 0;

#define PTR_TO_ARG(ptr) (QByteArray().setNum((qint64)(qptrdiff)(void*)(ptr)))
	QList<QByteArray> args;
	args << "-I" << "dummy" << "--ignore-config" << "--extraintf=logger"
		<< "--quiet"
//		<< "--verbose=2"
#ifndef Q_WS_X11
		<< "--no-xlib"
#endif
		<< "--reset-plugins-cache" << "--plugin-path" << Info::pluginPath()
		<< "--no-media-library" << "--no-osd" << "--no-sub-autodetect-file"
		<< "--no-stats" << "--no-video-title-show" << "--album-art=0"
		<< "--cmplayer-vout-chroma" << "AUTO"
		<< "--cmplayer-vout-cb-lock" << PTR_TO_ARG(cbVideoLock)
		<< "--cmplayer-vout-cb-unlock" << PTR_TO_ARG(cbVideoUnlock)
		<< "--cmplayer-vout-cb-display" << PTR_TO_ARG(cbVideoDisplay)
		<< "--cmplayer-vout-cb-render" << PTR_TO_ARG(cbVideoRender)
		<< "--cmplayer-vout-cb-prepare" << PTR_TO_ARG(cbVideoPrepare)
		<< "--cmplayer-vout-data" << PTR_TO_ARG(d)
		<< "--cmplayer-vout-util" << PTR_TO_ARG(&util_v)
		<< "--vout" << "cmplayer-vout"
		<< "--cmplayer-vfilter-data" << PTR_TO_ARG(d)
		<< "--cmplayer-vfilter-cb-process" << PTR_TO_ARG(cbVideoProcess)
		<< "--video-filter" << "cmplayer-vfilter"
		<< "--cmplayer-afilter-cb-prepare" << PTR_TO_ARG(cbAudioPrepare)
		<< "--cmplayer-afilter-cb-process" << PTR_TO_ARG(cbAudioProcess)
		<< "--cmplayer-afilter-data" << PTR_TO_ARG(d)
		<< "--cmplayer-afilter-util" << PTR_TO_ARG(&util_a)
		<< "--audio-filter"<< "cmplayer-afilter";
	const QByteArray add = qgetenv("CMPLAYER_VLC_OPTION");
	if (!add.isEmpty())
		args.append(add.split(','));
	qDebug() << "Iniitalize vlc with:";
	qDebug() << args;

	QVector<const char*> argv(args.size());
	for (int i=0; i<argv.size(); ++i)
		argv[i] = args[i].constData();
	m_inst = libvlc_new(argv.count(), argv.constData());
	m_mp = libvlc_media_player_new(m_inst);
	libvlc_video_set_mouse_input(m_mp, 0);
	libvlc_event_manager_t *man = libvlc_media_player_event_manager(m_mp);
	libvlc_event_type_t events[] = {
		libvlc_MediaPlayerTimeChanged,
		libvlc_MediaPlayerTitleChanged,
		libvlc_MediaPlayerSeekableChanged,
		libvlc_MediaPlayerLengthChanged,
		libvlc_MediaPlayerPlaying,
		libvlc_MediaPlayerPaused,
		libvlc_MediaPlayerEndReached,
		libvlc_MediaPlayerOpening,
		libvlc_MediaPlayerBuffering,
		libvlc_MediaPlayerStopped,
		libvlc_MediaPlayerEncounteredError,
	};
	int evCount = sizeof(events) / sizeof(*events);
	for (int i = 0; i < evCount; i++)
		libvlc_event_attach(man, events[i], cbManageEvent, d);

	m_engine = new PlayEngine(m_mp);
	m_video = new VideoRenderer(&util_v);
	m_audio = new AudioController(&util_a);
}

void LibVLC::finalize() {
	Q_ASSERT(d != 0);

	delete d;
	d = 0;

	m_engine->stop();
	libvlc_media_player_stop(m_mp);
	delete m_engine;
	delete m_video;
	delete m_audio;
	libvlc_media_player_release(m_mp);
	libvlc_release(m_inst);
}

void LibVLC::cbAudioPrepare(void *data, const AudioFormat *format) {
	Q_ASSERT(data == d);
	if (d && m_audio)
		m_audio->prepare(format);
}

AudioBuffer *LibVLC::cbAudioProcess(void *data, AudioBuffer *in) {
	Q_ASSERT(data == d);
	if (d && m_audio)
		return m_audio->process(in);
	return 0;
}

void LibVLC::cbVideoProcess(void *data, void **planes) {
	Q_ASSERT(data == d);
	if (d && m_video)
		m_video->process(planes);
}

void LibVLC::cbVideoRender(void *data, void **planes) {
	Q_ASSERT(data == d);
	if (d && m_video)
		m_video->render(planes);
}

void *LibVLC::cbVideoLock(void *data, void ***planes) {
	Q_ASSERT(data == d);
	if (d && m_video)
		return m_video->lock(planes);
	return 0;
}

void LibVLC::cbVideoUnlock(void *data, void *id, void *const *plane) {
	Q_ASSERT(data == d);
	if (d && m_video)
		m_video->unlock(id, plane);
}

void LibVLC::cbVideoDisplay(void *data, void *id) {
	Q_ASSERT(data == d);
	if (d && m_video)
		m_video->display(id);
}

void LibVLC::cbVideoPrepare(void *data, const VideoFormat *format) {
	Q_ASSERT(data == d);
	if (d && m_video)
		m_video->prepare(format);
}

void LibVLC::cbManageEvent(const libvlc_event_t *event, void *data) {
	Q_ASSERT(data == d);
	if (d && m_engine) {
		switch (event->type) {
		case libvlc_MediaPlayerSeekableChanged:
		case libvlc_MediaPlayerTimeChanged:
		case libvlc_MediaPlayerPlaying:
		case libvlc_MediaPlayerPaused:
		case libvlc_MediaPlayerEndReached:
		case libvlc_MediaPlayerOpening:
		case libvlc_MediaPlayerBuffering:
		case libvlc_MediaPlayerStopped:
		case libvlc_MediaPlayerEncounteredError:
		case libvlc_MediaPlayerLengthChanged:
		case libvlc_MediaPlayerTitleChanged:
			m_engine->parseEvent(event);
			break;
		default:
			break;
		}
	}
}


