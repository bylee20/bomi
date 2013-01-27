#include "playengine.hpp"
#include "videoframe.hpp"
#include "videooutput.hpp"
//#include "videorendereritem.hpp"
#include "playlistmodel.hpp"
#define PLAY_ENGINE_P
#include "mpcore.hpp"
#undef PLAY_ENGINE_P
#undef run_command
extern "C" {
#include <core/command.h>
#include <video/out/vo.h>
#include <core/playlist.h>
#include <core/codec-cfg.h>
#include <core/m_property.h>
#include <core/input/input.h>
#include <audio/filter/af.h>
#include <stream/stream.h>
}

enum MpCmd {MpSetProperty = -1};

struct mp_volnorm {int method;	float mul;};

#ifdef Q_OS_MAC
namespace std {
	void __throw_bad_function_call() {throw "bad";}
}
#endif

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

struct PlayEngine::Context {
	MPContext mp;
	PlayEngine *p;
};

template<typename T> static inline T &getCmdArg(mp_cmd *cmd, int idx = 0);
template<> inline float &getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.f;}
template<> inline int &getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.i;}
template<> inline char *&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.s;}

struct PlayEngine::Data {
	QMutex mutex;
	QByteArray fileName;
	QTimer ticker = {};
	bool quit = false, playing = false, init = false;
	int start = 0, tick = 0;
	bool wasPlaying = false;
	PlayEngine::Context *ctx = nullptr;
	MPContext *mpctx = nullptr;
	VideoOutput *video = nullptr;
	QMutex q_mutex;		QWaitCondition q_wait;
	GetStartTime getStartTimeFunc;
	PlaylistModel playlist;
	double videoAspect = 0.0;
	int getStartTime(const Mrl &mrl) {
		return getStartTimeFunc ? getStartTimeFunc(mrl) : 0;
	}

	QByteArray &setFileName(const Mrl &mrl) {
		fileName = "\"";
		fileName += mrl.toString().toLocal8Bit();
		fileName += "\"";
		return fileName;
	}

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
	connect(&d->playlist, &PlaylistModel::playRequested, [this] (int row) {
		load(row, d->getStartTime(d->playlist[row]));
	});

}

PlayEngine::~PlayEngine() {
	delete d->video;
	delete d;
}

void PlayEngine::setGetStartTimeFunction(const GetStartTime &func) {
	d->getStartTimeFunc = func;
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

bool PlayEngine::isHwAccActivated() const {
//#ifdef Q_OS_MAC
//	return d->mpctx && d->mpctx->sh_video && d->mpctx->sh_video->codec && !strcmp(d->mpctx->sh_video->codec->name, "ffh264vda");
//#endif
	return d->video->isHwAccActivated();
}

void PlayEngine::setVideoAspect(double ratio) {
	if (d->videoAspect != ratio)
		emit videoAspectRatioChanged(d->videoAspect = ratio);
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
	m_title = 0;
}

class StreamOpenEvent : public QEvent {
public:
	static constexpr int Type = QEvent::User+1;
	StreamOpenEvent(): QEvent((QEvent::Type)Type) {}
	Stream::Type stream() const {return m_stream;}
private:
	Stream::Type m_stream = Stream::Unknown;
};

void PlayEngine::customEvent(QEvent *event) {
	if (event->type() == StreamOpenEvent::Type) {
		emit aboutToPlay();
		emit started(d->playlist.loadedMrl());
		d->start = 0;
	}
}

void PlayEngine::setCurrentDvdTitle(int id) {
	auto mrl = d->playlist.loadedMrl();
	if (mrl.isDvd()) {
		const QString path = "dvd://" % QString::number(id) % mrl.toString().mid(6);
		d->fileName = path.toLocal8Bit();
		tellmp("loadfile", d->fileName, 0);
	}
}

bool PlayEngine::parse(const Id &id) {
	if (getStream(id, "AUDIO", "AID", m_audioStreams, tr("Audio %1")))
		return true;
	else if (getStream(id, "VIDEO", "VID", m_videoStreams, tr("Video %1")))
		return true;
	else if (getStream(id, "SUBTITLE", "SID", m_subtitleStreams, tr("Subtitle %1")))
		return true;
	else if (!id.name.isEmpty()) {
		if (id.name.startsWith(_L("DVD_"))) {
			auto dvd = id.name.midRef(4);
			if (_Same(dvd, "TITLES")) {
//				m_dvd.titles[id.value.toInt()];
			} else if(dvd.startsWith(_L("TITLE_"))) {
				auto title = _MidRef(dvd, 6);
				int idx = id.name.indexOf(_L("_"), title.position());
				if (idx != -1) {
					bool ok = false;
					int tid = id.name.mid(title.position(), idx-title.position()).toInt(&ok);
					if (ok) {
						auto var = id.name.midRef(idx+1);
						auto &title = m_dvd.titles[tid];
						title.number = tid;
						title.m_name = tr("Title %1").arg(tid);
						if (_Same(var, "CHAPTERS"))
							title.chapters = id.value.toInt();
						else if (_Same(var, "ANGLES"))
							title.angles = id.value.toInt();
						else if (_Same(var, "LENGTH"))
							title.length = id.value.toDouble()*1000+0.5;
						else
							return false;
					} else
						return false;
				} else
					return false;
			} else if (_Same(dvd, "VOLUME_ID")) {
				m_dvd.volume = id.value;
			} else if (_Same(dvd, "DISC_ID")) {
				m_dvd.id = id.value;
			} else if (_Same(dvd, "CURRENT_TITLE")) {
				m_title = id.value.toInt();
			} else
				return false;
			return true;
		} else if (id.name.startsWith("VIDEO")) {
			if (_Same(id.name, "VIDEO_ASPECT")) {
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
	static QRegExp rxTitle("^CHAPTERS: (.+)$");
	if (rxTitle.indexIn(line) != -1) {
		m_chapters.clear();
		for (auto name : rxTitle.cap(1).split(',', QString::SkipEmptyParts)) {
			Chapter chapter; chapter.m_name = name;
			m_chapters.append(chapter);
		}
	} else
//	if (_Same(line, "DVDNAV_TITLE_IS_MOVIE")) {
////		m_isInDvdMenu = false;
//	} else if (_Same(line, "DVDNAV_TITLE_IS_MENU")) {
////		m_isInDvdMenu = true;
//	} else if (line.startsWith(_L("DVDNAV, switched to title: "))) {
////		m_title = line.mid(27).toInt();
//	} else
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
	} else {
		::run_command(d->mpctx, cmd);
	}
}

bool PlayEngine::isInitialized() const {
	return d->init;
}

void PlayEngine::run() {
	QList<QByteArray> args;
	args << "cmplayer-mpv" << "--no-config=all" << "--idle" << "--no-fs"
		<< "--fixed-vo" << "--softvol=yes" << "--softvol-max=1000.0"
		<< "--no-autosub" << "--osd-level=0" << "--quiet" << "--identify"
		<< "--input=no-default-bindings" << "--no-consolecontrols" << "--no-mouseinput";
	const int argc = args.size();
	QSharedPointer<char*> argv(new char*[argc], [](char **argv){delete[] argv;});
	for (int i=0; i<argc; ++i)
		argv.data()[i] = args[i].data();
	d->ctx = reinterpret_cast<Context*>(talloc_named_const(0, sizeof(*d->ctx), "MPContext"));
	d->ctx->p = this;
	auto mpctx = d->mpctx = &d->ctx->mp;
	mpv_init(d->mpctx, argc, argv.data());
	vo_cmplayer = d->video->vo_create(d->mpctx);
	d->init = true;
	emit initialized();

	d->quit = false;

	auto idle = [this, mpctx] () {
		while (mpctx->opts.player_idle_mode && !mpctx->playlist->current && d->mpctx->stop_play != PT_QUIT) {
			uninit_player(mpctx, 0);
			mp_cmd_t *cmd;
			while (!(cmd = mp_input_get_cmd(mpctx->input, get_wakeup_period(d->mpctx)*1000, false)))
				;
			mpctx_run_command(mpctx, cmd);
			mp_cmd_free(cmd);
		}
	};
	while (!d->quit) {
		idle();
		if (mpctx->stop_play == PT_QUIT)
			break;
		Q_ASSERT(mpctx->playlist->current);
		clear();
		setState(EngineBuffering);
		emit aboutToOpen();
		auto error = prepare_to_play_current_file(mpctx);
		int terminated = 0, duration = 0;
		Mrl mrl = d->playlist.loadedMrl();
		if (error == NoMpError) {
			d->playing = true;
			emit seekableChanged(isSeekable());
			d->mpctx->opts.play_start.pos = d->start*1e-3;
			d->mpctx->opts.play_start.type = REL_TIME_ABSOLUTE;
			setmp("speed", (float)m_speed);
			mixer_setvolume(&d->mpctx->mixer, mpVolume(), mpVolume());
			mixer_setmute(&d->mpctx->mixer, m_muted);
			if (!m_af.isEmpty()) tellmp("af_add", m_af);
			qApp->postEvent(this, new StreamOpenEvent);
			auto err = start_to_play_current_file(mpctx);
			terminated = position();
			duration = this->duration();
			terminate_playback(mpctx, err);
			d->playing = false;
		} else
			setState(EngineError);
		if (mpctx->stop_play == PT_QUIT) {
			if (error == NoMpError) {
				setState(EngineStopped);
				emit stopped(d->playlist.loadedMrl(), terminated, duration);
			}
			break;
		}
		playlist_entry *entry = nullptr;
		switch (mpctx->stop_play) {
		case KEEP_PLAYING:
		case AT_END_OF_FILE: {// finished
			setState(EngineFinished);
			emit finished(mrl);
			playlist_clear(mpctx->playlist);
			if (d->playlist.hasNext()) {
				const auto prev = d->playlist.loadedMrl();
				d->playlist.setLoaded(d->playlist.next());
				const auto mrl = d->playlist.loadedMrl();
				if (prev != mrl)
					emit mrlChanged(mrl);
				d->start = d->getStartTime(mrl);
				playlist_add(mpctx->playlist, playlist_entry_new(mrl.toString().toLocal8Bit()));
				entry = mpctx->playlist->first;
			} else
				emit d->playlist.finished();
			break;
		} case PT_CURRENT_ENTRY: // stopped by loadfile
			entry = mpctx->playlist->current;
		default: // just stopped
			setState(EngineStopped);
			emit stopped(mrl, terminated, duration);
			break;
		}

		mpctx->playlist->current = entry;
		mpctx->playlist->current_was_replaced = false;
		mpctx->stop_play = KEEP_PLAYING;
		if (!mpctx->playlist->current && !mpctx->opts.player_idle_mode)
			break;
	}
	d->video->release();
	mpctx_delete(d->mpctx);
	d->mpctx = nullptr;
	vo_cmplayer = nullptr;
	emit finalized();
}

void PlayEngine::tellmp(const QString &cmd) {
	if (d->mpctx && d->mpctx->input) {
		mp_input_queue_cmd(d->mpctx->input, mp_input_parse_cmd(bstr0(cmd.toLocal8Bit().data()), ""));
	}
}

void PlayEngine::quit() {
	tellmp("quit 1");
}

void PlayEngine::play(int time) {
	d->start = time;
	tellmp("loadfile", d->setFileName(d->playlist.loadedMrl()), 0);
}

bool PlayEngine::load(int row, int start) {
	if (!d->playlist.isValidRow(row))
		return false;
	if (d->playlist.loaded() == row) {
		if (start >= 0 && !d->playing)
			play(start);
	} else {
		d->playlist.setLoaded(row);
		emit mrlChanged(d->playlist.loadedMrl());
		if (start < 0)
			stop();
		else
			play(start);
	}
	return true;
}

void PlayEngine::load(const Mrl &mrl, bool play) {
	load(mrl, play ? d->getStartTime(mrl) : -1);
}

void PlayEngine::load(const Mrl &mrl, int start) {
	auto row = d->playlist.rowOf(mrl);
	if (row < 0)
		row = d->playlist.append(mrl);
	load(row, start);
}

int PlayEngine::position() const {
	return d->mpctx && d->mpctx->demuxer ? get_current_time(d->mpctx)*1000.0 + 0.5 : 0;
}

bool PlayEngine::isSeekable() const {
	return d->mpctx && d->mpctx->stream && d->mpctx->stream->seek && (!d->mpctx->demuxer || d->mpctx->demuxer->seekable);
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
		play(d->getStartTime(d->playlist.loadedMrl()));
		break;
	default:
		break;
	}
}

void PlayEngine::setPlaylist(const Playlist &playlist) {
	d->playlist.setPlaylist(playlist);
}

Mrl PlayEngine::mrl() const {
	return d->playlist.loadedMrl();
}

QString PlayEngine::mediaName() const {
	const auto mrl = d->playlist.loadedMrl();
	if (mrl.isLocalFile())
		return tr("File") % _L(": ") % QFileInfo(mrl.toLocalFile()).fileName();
	if (mrl.isDvd() && !m_dvd.volume.isEmpty())
		return _L("DVD") % _L(": ") % m_dvd.volume;
	return _L("URL") % _L(": ") % mrl.toString();
}

int PlayEngine::currentAudioStream() const {
	if (d->mpctx && d->mpctx->sh_audio)
		return d->mpctx->sh_audio->aid;
	return -1;
}

int PlayEngine::currentVideoStream() const {
	return hasVideo() ? d->mpctx->sh_video->vid : -1;
}

const PlaylistModel &PlayEngine::playlist() const {
	return d->playlist;
}

PlaylistModel &PlayEngine::playlist() {
	return d->playlist;
}

double PlayEngine::fps() const {
	return hasVideo() ? d->mpctx->sh_video->fps : 25;
}

void PlayEngine::setVideoRenderer(VideoRendererItem *renderer) {
	if (_Change(m_renderer, renderer)) {
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
}
