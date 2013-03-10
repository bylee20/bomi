#include "playengine.hpp"
#include "info.hpp"
#include "videoframe.hpp"
#include "videooutput.hpp"
#include "hwacc.hpp"
#include "audiocontroller.hpp"
#include "playlistmodel.hpp"
#include "dataevent.hpp"
#define PLAY_ENGINE_P
#include "mpcore.hpp"
#undef PLAY_ENGINE_P
#undef run_command
extern "C" {
#include <core/command.h>
#include <video/out/vo.h>
#include <video/decode/vd.h>
#include <core/playlist.h>
#include <core/codecs.h>
#include <core/m_property.h>
#include <core/input/input.h>
#include <audio/filter/af.h>
#include <stream/stream.h>
}

enum EventType {
	UserType = QEvent::User, StreamOpen, StateChange, MrlStopped, MrlFinished, PlaylistFinished, MrlChanged
};

enum MpCmd {MpSetProperty = -1, MpResetAudioChain = -2};

struct mp_volnorm {int method;	float mul; float avg;};

struct PlayEngine::Context { MPContext mp; PlayEngine *p; };

template<typename T> static inline T &getCmdArg(mp_cmd *cmd, int idx = 0);
template<> inline float	&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.f;}
template<> inline int	&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.i;}
template<> inline char*	&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.s;}

struct PlayEngine::Data {
	Data(PlayEngine *engine): p(engine) {}
	PlayEngine *p = nullptr;
	AudioController *audio = nullptr;
	QByteArray fileName;
	QTimer ticker;
	bool quit = false, playing = false, init = false;
	int start = 0, tick = 0;
	PlayEngine::Context *ctx = nullptr;
	MPContext *mpctx = nullptr;
	VideoOutput *video = nullptr;
	GetStartTime getStartTimeFunc;
	PlaylistModel playlist;
	double videoAspect = 0.0;
	QByteArray hwAccCodecs;
	QMutex imgMutex;

	int getStartTime(const Mrl &mrl) {return getStartTimeFunc ? getStartTimeFunc(mrl) : 0;}
	QByteArray &setFileName(const Mrl &mrl) {
		fileName = "\"";
		fileName += mrl.toString().toLocal8Bit();
		fileName += "\"";
		return fileName;
	}
	template<typename T>
	bool enqueue(int id, const char *name = "", const T &v = 0) {
		const bool ret = mpctx && mpctx->input && playing;
		if (ret) {
			mp_cmd_t *cmd = (mp_cmd_t*)talloc_ptrtype(NULL, cmd);
			cmd->id = id;
			cmd->name = (char*)name;
			getCmdArg<T>(cmd) = v;
			mp_input_queue_cmd(mpctx->input, cmd);
		}
		return ret;
	}
};

PlayEngine::PlayEngine()
: d(new Data(this)) {
	d->audio = new AudioController(this);
	d->video = new VideoOutput(this);
	mpctx_paused_changed = onPausedChanged;
	mpctx_play_started = onPlayStarted;
	connect(&d->ticker, &QTimer::timeout, [this] () {
		if (m_imgMode)
			emit tick(m_imgPos);
		else if (d->mpctx && (isPaused() || isPlaying())) {
			const int duration = qRound(get_time_length(d->mpctx)*1000.0);
			if (m_duration != duration)
				emit durationChanged(m_duration = duration);
			emit tick(position());
		}
	});
	d->ticker.setInterval(20);
	d->ticker.start();

	connect(d->video, &VideoOutput::formatChanged, this, &PlayEngine::videoFormatChanged);
	connect(&d->playlist, &PlaylistModel::playRequested, [this] (int row) {
		load(row, d->getStartTime(d->playlist[row]));
	});
}

PlayEngine::~PlayEngine() {
	delete d->audio;
	delete d->video;
	delete d;
}

void PlayEngine::seek(int pos) {
	if (m_imgMode) {
		if (m_imgDuration > 0) {
			d->imgMutex.lock();
			m_imgSeek = pos;
			d->imgMutex.unlock();
		}
	} else
		tellmp("seek", (double)pos/1000.0, 2);
}

void PlayEngine::relativeSeek(int pos) {
	if (m_imgMode) {
		if (m_imgDuration > 0) {
			d->imgMutex.lock();
			m_imgRelSeek += pos;
			d->imgMutex.unlock();
		}
	} else
		tellmp("seek", (double)pos/1000.0, 0);
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
	reinterpret_cast<Context*>(mpctx)->p->setState(mpctx->paused ? EnginePaused : EnginePlaying);
}

double PlayEngine::volumeNormalizer() const {
	auto amp = d->audio->normalizer();
	return amp < 0 ? 1.0 : amp;
}

bool PlayEngine::isHwAccActivated() const {
	if (d->mpctx && d->mpctx->sh_video && d->mpctx->sh_video->vd_driver)
		return qstrcmp(d->mpctx->sh_video->vd_driver->name, HwAcc::name()) == 0;
	return false;
}

void PlayEngine::setHwAccCodecs(const QList<int> &codecs) {
	d->hwAccCodecs.clear();
	for (auto id : codecs) {
		if (const char *name = HwAcc::codecName((AVCodecID)id)) {
			d->hwAccCodecs.append(name);
			d->hwAccCodecs.append(',');
		}
	}
	d->hwAccCodecs.chop(1);
}

void PlayEngine::setVideoAspect(double ratio) {
	if (d->videoAspect != ratio)
		emit videoAspectRatioChanged(d->videoAspect = ratio);
}

void PlayEngine::setCurrentSubtitleStream(int id) {
	if (id < 0) {
		setmp("sub-visibility", false);
		setmp("sub", id);
		m_subId = -1;
	} else {
		setmp("sub-visibility", true);
		setmp("sub", id);
		m_subId = id;
	}
}

int PlayEngine::currentSubtitleStream() const {
	return m_subId;
}

void PlayEngine::clear() {
	setVideoAspect(0.0);
	m_dvd.clear();
	m_audioStreams.clear();
	m_videoStreams.clear();
	m_subtitleStreams.clear();
	m_title = 0;
	m_subId = -1;
}

void PlayEngine::customEvent(QEvent *event) {
	switch ((int)event->type()) {
	case StreamOpen:
		if (!m_subtitleStreams.isEmpty())
			m_subtitleStreams[-1].m_name = tr("No Subtitle");
		emit seekableChanged(isSeekable());
		emit started(d->playlist.loadedMrl());
		d->start = 0;
		break;
	case StateChange: {
		EngineState state = EngineStopped;
		get(event, state);
		if (_Change(m_state, state))
			emit stateChanged(m_state);
		break;
	} case MrlStopped: {
		Mrl mrl; int terminated = 0, duration = 0;
		get(event, mrl, terminated, duration);
		emit stopped(mrl, terminated, duration);
		break;
	} case MrlFinished: {
		Mrl mrl; get(event, mrl);
		emit finished(mrl);
		break;
	} case PlaylistFinished:
		emit d->playlist.finished();
		break;
	case MrlChanged: {
		Mrl mrl; get(event, mrl);
		emit mrlChanged(mrl);
	}default:
		break;
	}
}

void PlayEngine::setState(EngineState state) {
	post(this, StateChange, state);
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
						title.m_id = tid;
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
			chapter.m_id = m_chapters.size();
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
		case MpResetAudioChain:
			reinit_audio_chain(d->mpctx);
		default:
			break;
		}
		cmd->id = MP_CMD_IGNORE;
	} else {
		::run_command(d->mpctx, cmd);
	}
}

class TimeLine {
public:
	TimeLine() {m_time.start();}
	int pos() const {return m_running ? m_pos + m_time.elapsed() : m_pos;}
	void stop () { if (m_running) { m_pos += m_time.elapsed(); m_running = false; } }
	void moveBy(int diff) {
		m_pos += diff;
		if (m_running) {
			if (m_pos + m_time.elapsed() < 0)
				m_pos = -m_time.elapsed();
		} else {
			if (m_pos < 0)
				m_pos = 0;
		}
	}
	void start() { if (!m_running) { m_time.restart(); m_running = true; } }
private:
	QTime m_time;
	int m_pos = 0;
	bool m_running = true;
};

bool PlayEngine::isInitialized() const {
	return d->init;
}

int PlayEngine::runImage(const Mrl &mrl, int &terminated, int &duration) {
	QImage image;
	if (!image.load(mrl.toLocalFile()))
		return OpenFileError;
	auto mpctx = &d->ctx->mp;
	auto error = prepare_to_play_current_file(mpctx);
	if (error != NoMpError)
		return error;
	d->video->output(image);
	m_imgPos = 0;
	post(this, StreamOpen);
	TimeLine time;
	while (!mpctx->stop_play) {
		mp_cmd_t *cmd = nullptr;
		while ((cmd = mp_input_get_cmd(mpctx->input, 0, 1)) != NULL) {
			cmd = mp_input_get_cmd(mpctx->input, 0, 0);
			runCommand(cmd);
			mp_cmd_free(cmd);
			if (mpctx->stop_play)
				break;
		}
		d->imgMutex.lock();
		if (m_imgSeek > 0)
			m_imgRelSeek = m_imgSeek - m_imgPos;
		if (m_imgRelSeek)
			time.moveBy(m_imgRelSeek);
		m_imgSeek = m_imgRelSeek = 0;
		d->imgMutex.unlock();
		if (m_imgDuration > 0) {
			if (isPaused())
				time.stop();
			else {
				time.start();
				m_imgPos = qBound(0, time.pos(), m_imgDuration);
			}
			if (m_imgPos >= m_imgDuration)
				break;
		}
		msleep(50);
	}
	m_imgPos = 0;
	terminated = duration = 0;
//	mpctx->stop_play = AT_END_OF_FILE;
	return terminate_playback(mpctx, error);
}

int PlayEngine::runAv(const Mrl &/*mrl*/, int &terminated, int &duration) {
	auto mpctx = &d->ctx->mp;
	mpctx->opts.video_decoders = d->hwAccCodecs.data();
	d->mpctx->opts.play_start.pos = d->start*1e-3;
	d->mpctx->opts.play_start.type = REL_TIME_ABSOLUTE;
	setmp("speed", (float)m_speed);
	mixer_setvolume(&d->mpctx->mixer, mpVolume(), mpVolume());
	mixer_setmute(&d->mpctx->mixer, m_muted);
	auto error = prepare_to_play_current_file(mpctx);
	if (error == NoMpError) {
		post(this, StreamOpen);
		while (!mpctx->stop_play)
			run_playloop(mpctx);
		terminated = position();
		duration = this->duration();
		terminate_playback(mpctx, error);
	}
	return error;
}

void PlayEngine::run() {
	CharArrayList args = QStringList()
		<< "cmplayer-mpv" << "--no-config" << "--idle" << "--no-fs"
		<< ("--af=dummy=" % QString::number((quint64)(quintptr)(void*)d->audio))
		<< ("--vo=null:" % QString::number((quint64)(quintptr)(void*)(d->video)))
		<< "--fixed-vo" << "--softvol=yes" << "--softvol-max=1000.0"
		<< "--no-autosub" << "--osd-level=0" << "--quiet" << "--identify"
		<< "--no-consolecontrols" << "--no-mouseinput";
	d->ctx = reinterpret_cast<Context*>(talloc_named_const(0, sizeof(*d->ctx), "MPContext"));
	d->ctx->p = this;
	auto mpctx = d->mpctx = &d->ctx->mp;
	mpv_init(d->mpctx, args.size(), args.data());
	d->init = true;
	HwAcc hwAcc; (void)hwAcc;
	d->quit = false;
	while (!d->quit) {
		m_imgMode = false;
		idle_loop(mpctx);
		if (mpctx->stop_play == PT_QUIT)
			break;
		Q_ASSERT(mpctx->playlist->current);
		clear();
		int terminated = 0, duration = 0;
		Mrl mrl = d->playlist.loadedMrl();
		d->playing = true;
		setState(EngineBuffering);
		int error = NoMpError;
		m_imgMode = mrl.isImage();
		if (m_imgMode)
			error = runImage(mrl, terminated, duration);
		else
			error = runAv(mrl, terminated, duration);
		if (error != NoMpError)
			setState(EngineError);
		d->playing = false;
		qDebug() << "terminate playback";
		if (mpctx->stop_play == PT_QUIT) {
			if (error == NoMpError) {
				setState(EngineStopped);
				post(this, MrlStopped, d->playlist.loadedMrl(), terminated, duration);
			}
			break;
		}
		playlist_entry *entry = nullptr;
		switch (mpctx->stop_play) {
		case KEEP_PLAYING:
		case AT_END_OF_FILE: {// finished
			setState(EngineFinished);
			post(this, MrlFinished, mrl);
			playlist_clear(mpctx->playlist);
			if (d->playlist.hasNext()) {
				const auto prev = d->playlist.loadedMrl();
				d->playlist.setLoaded(d->playlist.next());
				const auto mrl = d->playlist.loadedMrl();
				if (prev != mrl)
					post(this, MrlChanged, mrl);
				d->start = d->getStartTime(mrl);
				playlist_add(mpctx->playlist, playlist_entry_new(mrl.toString().toLocal8Bit()));
				entry = mpctx->playlist->first;
			} else
				post(this, PlaylistFinished);
			break;
		} case PT_CURRENT_ENTRY: // stopped by loadfile
			entry = mpctx->playlist->current;
		default: // just stopped
			setState(EngineStopped);
			post(this, MrlStopped, mrl, terminated, duration);
			break;
		}

		mpctx->playlist->current = entry;
		mpctx->playlist->current_was_replaced = false;
		mpctx->stop_play = KEEP_PLAYING;
		if (!mpctx->playlist->current && !mpctx->opts.player_idle_mode)
			break;
	}
	qDebug() << "terminate loop";
	d->video->quit();
	mpctx->opts.video_decoders = nullptr;
	mpctx_delete(d->mpctx);
	d->mpctx = nullptr;
	d->init = false;
	qDebug() << "terminate engine";
}

void PlayEngine::tellmp(const QString &cmd) {
	if (d->mpctx && d->mpctx->input) {
		mp_input_queue_cmd(d->mpctx->input, mp_input_parse_cmd(bstr0(cmd.toLocal8Bit().data()), ""));
	}
}

void PlayEngine::quit() {
	d->video->quit();
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
		post(this, MrlChanged, d->playlist.loadedMrl());
		if (start < 0)
			stop();
		else
			play(start);
	}
	return true;
}

void PlayEngine::reload() {
	play(position());
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
	return m_imgMode ? m_imgPos : (d->mpctx && d->mpctx->demuxer ? get_current_time(d->mpctx)*1000.0 + 0.5 : 0);
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

void PlayEngine::pause() {
	if (!isPaused()) {
		if (m_imgMode)
			setState(EnginePaused);
		else
			tellmp("pause");
	}
}

void PlayEngine::play() {
	switch (m_state) {
	case EnginePaused:
		if (m_imgMode)
			setState(EnginePlaying);
		else
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
	if (_Change(m_renderer, renderer))
		d->video->setRenderer(m_renderer);
}

VideoFormat PlayEngine::videoFormat() const {
	return d->video->format();
}

void PlayEngine::setVolumeNormalized(bool on) {
	if (d->audio->setNormalizer(on))
		emit volumeNormalizedChanged(on);
}

void PlayEngine::setTempoScaled(bool on) {
	if (d->audio->setScaletempo(on)) {
		if (d->playing)
			d->enqueue<int>(MpResetAudioChain);
		emit tempoScaledChanged(on);
	}
}

bool PlayEngine::isVolumeNormalized() const {
	return d->audio->normalizer() > 0;
}

bool PlayEngine::isTempoScaled() const {
	return d->audio->scaletempo();
}

void PlayEngine::stop() {
	tellmp("stop");
}

void PlayEngine::setVolumeNormalizer(double target, double silence, double min, double max) {
	d->audio->setNormalizer(target, silence, min, max);
}
