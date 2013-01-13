#include "playengine.hpp"
#include "videoframe.hpp"
#include "videooutput.hpp"
#include "videorendereritem.hpp"
#define PLAY_ENGINE_P
#include "mpcore.hpp"
#undef PLAY_ENGINE_P
#undef run_command
extern "C" {
#include <core/command.h>
#include <core/playlist.h>
#include <core/codec-cfg.h>
#include <core/m_property.h>
#include <core/input/input.h>
#include <audio/filter/af.h>
#include <stream/stream.h>
}

enum MpCmd {MpSetProperty = -1};

struct mp_volnorm {int method;	float mul;};

static double volnorm_mul(MPContext *mpctx) {
	if (mpctx && mpctx->mixer.afilter) {
		auto af = mpctx->mixer.afilter->first;
		while (af) {
			if (strcmp(af->info->name, "volnorm") == 0)
				break;
			af = af->next;
		}
		if (af)
			return reinterpret_cast<mp_volnorm*>(af->setup)->mul;
	}
	return 1.0;
}

PlayEngine *PlayEngine::obj = nullptr;

struct PlayEngine::Context {
	MPContext mp;
	PlayEngine *p;
};

template<typename T> static inline T &getCmdArg(mp_cmd *cmd, int idx = 0);
template<> inline float &getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.f;}
template<> inline int &getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.i;}
template<> inline char *&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.s;}

struct PlayEngine::Data {
	Mrl playingMrl;
	QByteArray fileName;
	QTimer ticker = {};
	bool quit = false, playing = false, idling = false, init = false;
	int initSeek = 0, start = 0;
	bool wasPlaying = false;
	PlayEngine::Context *ctx = nullptr;
	MPContext *mpctx = nullptr;
	VideoOutput *video = nullptr;
	QMutex q_mutex;		QWaitCondition q_wait;

	double videoAspect = 0.0;

	template<typename T>
	bool enqueue(int id, const char *name, const T &v) {
		const bool ret = mpctx && mpctx->input && playing;
		if (ret) {
			mp_cmd_t *cmd = talloc_ptrtype(NULL, cmd);
			cmd->id = id;
			cmd->name = (char*)name;
			getCmdArg<T>(cmd) = v;
			mp_input_queue_cmd(mpctx->input, cmd);
		}
		return ret;
	}
};

PlayEngine::PlayEngine()
: d(new Data) {
	d->video = new VideoOutput(this);
	mpctx_paused_changed = onPausedChanged;
	mpctx_play_started = onPlayStarted;
	connect(&d->ticker, &QTimer::timeout, [this] () {
		if (d->mpctx && (isPaused() || isPlaying())) {
			const int duration = qRound(get_time_length(d->mpctx)*1000.0);
			if (m_duration != duration)
				emit durationChanged(m_duration = duration);
			emit tick(position());
		}
	});
	d->ticker.setInterval(20);
	d->ticker.start();

	connect(d->video, &VideoOutput::formatChanged, this, &PlayEngine::videoFormatChanged, Qt::QueuedConnection);
}

PlayEngine::~PlayEngine() {
	delete d->video;
	delete d;
}

void PlayEngine::setmp(const char *name, int value) {
	d->enqueue<int>(MpSetProperty, name, value);
}

void PlayEngine::setmp(const char *name, float value) {
	d->enqueue<float>(MpSetProperty, name, value);
}

void PlayEngine::onPlayStarted(MPContext *mpctx) {
	reinterpret_cast<Context*>(mpctx)->p->setState(EnginePlaying);
}

double PlayEngine::volumeNormalizer() const {
	return volnorm_mul(d->mpctx);
}

bool PlayEngine::usingHwAcc() const {
#ifdef Q_OS_MAC
	return d->mpctx && d->mpctx->sh_video && d->mpctx->sh_video->codec && !strcmp(d->mpctx->sh_video->codec->name, "ffh264vda");
#endif
	return d->video->usingHwAccel();
}

void PlayEngine::setVideoAspect(double ratio) {
	if (d->videoAspect != ratio)
		emit videoAspectRatioChanged(d->videoAspect = ratio);
	if (m_renderer)
		m_renderer->setVideoAspectRaito(ratio);
}

int PlayEngine::currentSubtitleStream() const {
	return -1;
//	return d->mpctx->global_sub_pos;
}

void PlayEngine::clear() {
	setVideoAspect(0.0);
	m_dvd.clear();
	m_audioStreams.clear();
	m_videoStreams.clear();
	m_subtitleStreams.clear();
//	emit audioStreamFound(m_audioStreams);
//	emit audioStreamFound(m_videoStreams);
//	emit audioStreamFound(m_subtitleStreams);
	m_title = 0;
}

class StreamChangeEvent : public QEvent {
public:
	static constexpr int Type = QEvent::User+1;
	StreamChangeEvent(Stream::Type stream): QEvent((QEvent::Type)Type), m_stream(stream) {}
	Stream::Type stream() const {return m_stream;}
private:
	Stream::Type m_stream = Stream::Unknown;
};

void PlayEngine::customEvent(QEvent *event) {
	if (event->type() == StreamChangeEvent::Type) {
		auto stream = static_cast<StreamChangeEvent*>(event)->stream();
		if (stream == Stream::Audio)
			emit audioStreamFound(m_audioStreams);
		else if (stream == Stream::Video)
			emit videoStreamFound(m_videoStreams);
		else if (stream == Stream::Subtitle)
			emit subtitleStreamFound(m_subtitleStreams);
	}
}

bool PlayEngine::parse(const Id &id) {
	if (getStream(id, "AUDIO", "AID", m_audioStreams))
		qApp->postEvent(this, new StreamChangeEvent(Stream::Audio));
	else if (getStream(id, "VIDEO", "VID", m_videoStreams))
		qApp->postEvent(this, new StreamChangeEvent(Stream::Video));
	else if (getStream(id, "SUBTITLE", "SID", m_subtitleStreams))
		qApp->postEvent(this, new StreamChangeEvent(Stream::Subtitle));
	else if (!id.name.isEmpty()) {
		if (id.name.startsWith(_L("DVD_"))) {
			if (same(id.name, "DVD_VOLUME_ID")) {
				m_dvd.volume = id.value;
				m_dvd.titles[0].name = tr("DVD Menu");
			} else if (same(id.name, "DVD_CURRENT_TITLE")) {
				m_title = id.value.toInt();
			} else
				return false;
			return true;
		} else if (id.name.startsWith("VIDEO")) {
			if (same(id.name, "VIDEO_ASPECT")) {
				setVideoAspect(id.value.toDouble());
			} else
				return false;
			return true;
		}
//		static QRegExp rxTitle("DVD_TITLE_(\\d+)_(LENGTH|CHAPTERS)");
	} else
		return false;
	return true;
}

bool PlayEngine::parse(const QString &line) {
	static QRegExp rxTitle("^TITLE (\\d+), CHAPTERS: (.+)$");
	if (rxTitle.indexIn(line) != -1) {
		const int idx = rxTitle.cap(1).toInt();
		auto &title = m_dvd.titles[idx];
		title.name = tr("Title %1").arg(idx);
		title.chapters.clear();
		title.chapters << "00:00:00";
		title.chapters += rxTitle.cap(2).split(',', QString::SkipEmptyParts);
		title.chapters.pop_back();
	} else if (same(line, "DVDNAV_TITLE_IS_MOVIE")) {
//		m_isInDvdMenu = false;
	} else if (same(line, "DVDNAV_TITLE_IS_MENU")) {
//		m_isInDvdMenu = true;
	} else if (line.startsWith(_L("DVDNAV, switched to title: "))) {
		m_title = line.mid(27).toInt();
	} else
		return false;
	return true;
}



MPContext *PlayEngine::context() const {
	return d->mpctx;
}

double PlayEngine::videoAspectRatio() const {
	return d->videoAspect;
}

extern "C" void mpctx_run_command(MPContext *mpctx, mp_cmd *cmd) {((PlayEngine::Context*)(mpctx))->p->runCommand(cmd);}
extern "C" void run_command(MPContext *mpctx, mp_cmd *cmd);
void PlayEngine::runCommand(mp_cmd *cmd) {
	if (cmd->id < 0) {
		switch (cmd->id) {
		case MpSetProperty:
			mp_property_do(cmd->name, M_PROPERTY_SET, &cmd->args[0].v.f, d->mpctx);
			break;
		default:
			break;
		}
		cmd->id = MP_CMD_IGNORE;
	} else
		run_command(d->mpctx, cmd);
}

bool PlayEngine::isInitialized() const {
	return d->init;
}

void PlayEngine::run() {
	QList<QByteArray> args;
	args << "cmplayer-mplayer2" << "--no-config=all" << "--idle" << "--no-fs" << "--fixed-vo" << "--softvol=yes" << "--softvol-max=1000.0"
		<< "--no-autosub" << "--osd-level=0" << "--quiet" << "--identify"
		<< "--input=no-default-bindings" << "--no-consolecontrols" << "--no-mouseinput";
	const int argc = args.size();
	QSharedPointer<char*> argv(new char*[argc], [](char **argv){delete[] argv;});
	for (int i=0; i<argc; ++i)
		argv.data()[i] = args[i].data();
	d->ctx = reinterpret_cast<Context*>(talloc_named_const(0, sizeof(*d->ctx), "MPContext"));
	d->ctx->p = this;
	auto mpctx = d->mpctx = &d->ctx->mp;
	qDebug() << "mpctx_init" << mpv_init(d->mpctx, argc, argv.data());
	vo_cmplayer = d->video->vo_create(d->mpctx);
	d->init = true;
	emit initialized();

	d->quit = false;

	auto idle = [this, mpctx] () {
		while (mpctx->opts.player_idle_mode && !mpctx->playlist->current && d->mpctx->stop_play != PT_QUIT) {
			uninit_player(mpctx, INITIALIZED_AO | INITIALIZED_VO);
			mp_cmd_t *cmd;
			while (!(cmd = mp_input_get_cmd(mpctx->input, get_wakeup_period(d->mpctx)*1000, false)))
				;
			run_command(mpctx, cmd);
			mp_cmd_free(cmd);
		}
	};
	for (;;) {
		idle();
		if (mpctx->stop_play == PT_QUIT)
			break;
		Q_ASSERT(mpctx->playlist->current);
		clear();
		setState(EngineBuffering);
		emit aboutToOpen();
		//		d->playing = true;
		//		d->playingMrl = m_mrl;
		auto error = prepare_to_play_current_file(mpctx);
		if (error == NoMpError) {
			d->playing = true;
			setMpVolume();
			//			tellmp("speed_set", m_speed);
			//			if (d->initSeek > 0)
			//				d->mpctx->opts.seek_to_sec = (double)d->initSeek/1000.0;
			emit seekableChanged(isSeekable());

			//			auto mixer = &d->mpctx->mixer;
			//	//		mixer_setvolume2(mixer, 0.0f, 0.0f); mplayer bug? when volume < 1.0, no sound output
			//			mixer_setmute(mixer, m_muted);
			//			if (!m_af.isEmpty())
			//				tellmp("af_add", m_af);

			emit aboutToPlay();
			auto error = start_to_play_current_file(mpctx);
			d->playing = false;
			if (error != NoMpError)
				setState(EngineError);

		} else
			setState(EngineError);
		if (mpctx->stop_play == PT_QUIT) {
			if (d->wasPlaying)
				setState(EngineStopped);
			break;
		}
		playlist_entry *new_entry = nullptr;
		switch (mpctx->stop_play) {
		case KEEP_PLAYING:
		case AT_END_OF_FILE: // finished
			setState(EngineFinished);
			playlist_clear(mpctx->playlist);
			break;
		case PT_CURRENT_ENTRY: // stopped by loadfile
			new_entry = mpctx->playlist->current;
		default: // just stopped
			setState(EngineStopped);
			break;
		}
		mpctx->playlist->current = new_entry;
		mpctx->playlist->current_was_replaced = false;
		mpctx->stop_play = KEEP_PLAYING;
		if (!mpctx->playlist->current && !mpctx->opts.player_idle_mode)
			break;
	}
	qDebug() << "finalization started";
	mpctx_delete(d->mpctx);
	d->mpctx = nullptr;
	qDebug() << "mpctx deleted";
	vo_cmplayer = nullptr;
	emit finalized();


	//		if (d->quit && d->mpctx->stop_play == KEEP_PLAYING)
	//			setState(EngineStopped);
	//		mpctx_cleanup_playback(d->mpctx);
	//		d->fileName = d->nextFileName;
	//		d->nextFileName.clear();
	//		if (d->quit) {
	//			qDebug() << "quit!";
	//			break;
	//		}
	//	}


}

void PlayEngine::tellmp(const QString &cmd) {
	if (d->mpctx && d->mpctx->input) {
		mp_input_queue_cmd(d->mpctx->input, mp_input_parse_cmd(bstr0(cmd.toLocal8Bit().data()), ""));
	}
}

void PlayEngine::quitRunning() {
	tellmp("quit 1");
	d->quit = true;
}

void PlayEngine::play(int time) {
	d->initSeek = time;
	d->fileName = "\"";
	d->fileName += m_mrl.toString().toLocal8Bit();
	d->fileName += '"';
	tellmp("loadfile", d->fileName, 0);
}

void PlayEngine::setMrl(const Mrl &mrl, int start, bool play) {
	if (mrl != m_mrl) {
		m_mrl = mrl;
		d->start = start;
		emit mrlChanged(m_mrl);
		if (play)
			this->play(d->start);
	}
}

int PlayEngine::position() const {
	return d->mpctx && d->mpctx->demuxer ? get_current_time(d->mpctx)*1000.0 + 0.5 : 0;
}

bool PlayEngine::isSeekable() const {
	return d->mpctx && d->mpctx->stream && d->mpctx->stream->seek && (!d->mpctx->demuxer || d->mpctx->demuxer->seekable);
}

void PlayEngine::setSpeed(double speed) {
	if (m_speed != speed) {
		m_speed = speed;
		if (d->playing)
			tellmp("speed_set", speed);
	}
}

bool PlayEngine::hasVideo() const {
	return d->mpctx && d->mpctx->sh_video;
}

bool PlayEngine::atEnd() const {
	return d->mpctx->stop_play == AT_END_OF_FILE;
}


int PlayEngine::currentChapter() const {
	if (d->playing)
		return get_current_chapter(d->mpctx);
	return -2;
}


void PlayEngine::onPausedChanged(MPContext *mpctx) {
	PlayEngine *engine = reinterpret_cast<Context*>(mpctx)->p;
	if (mpctx->stop_play == KEEP_PLAYING)
		engine->setState(mpctx->paused ? EnginePaused : EnginePlaying);
}

void PlayEngine::play() {
	switch (m_state) {
	case EnginePaused:
		tellmp("pause");
		break;
	case EngineStopped:
	case EngineFinished:
		play(d->start);
		break;
	default:
		break;
	}
}

QString PlayEngine::mediaName() const {
	if (m_mrl.isLocalFile())
		return tr("File") % _L(": ") % QFileInfo(m_mrl.toLocalFile()).fileName();
	if (m_mrl.isDvd() && !m_dvd.volume.isEmpty())
		return _L("DVD") % _L(": ") % m_dvd.volume;
	return _L("URL") % _L(": ") % m_mrl.toString();
}

int PlayEngine::currentAudioStream() const {
	if (d->mpctx && d->mpctx->sh_audio)
		return d->mpctx->sh_audio->aid;
	return -1;
}

int PlayEngine::currentVideoStream() const {
	return hasVideo() ? d->mpctx->sh_video->vid : -1;
}

double PlayEngine::fps() const {
	return hasVideo() ? d->mpctx->sh_video->fps : 25;
}

void PlayEngine::setVideoRenderer(VideoRendererItem *renderer) {
	if (_Change(m_renderer, renderer)) {
		setVideoAspect(d->videoAspect);
	}
}

VideoFormat PlayEngine::videoFormat() const {
	return d->video->format();
}

void PlayEngine::setAudioFilter(const QString &af, bool on) {
	auto it = qFind(m_af.begin(), m_af.end(), af);	const bool prev = it != m_af.end();
	if (prev != on) {
		if (on) {m_af.append(af); tellmp("af_add", af);}
		else {m_af.erase(it); tellmp("af_del", af);}
		emit audioFilterChanged(af, on);
	}
}

void PlayEngine::stop() {
	tellmp("stop");
//	enqueue(new Cmd(Cmd::Stop));
}
