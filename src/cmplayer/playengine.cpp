#include "playengine.hpp"
#include "videoframe.hpp"
#include "videooutput.hpp"
#include "videorendereritem.hpp"
#include "mpcore.hpp"

enum MpError {None = 0, UserInterrupted, CannotOpenStream, NoStream, InitVideoFilter, InitAudioFilter};
extern "C" {
#include <input/input.h>
#include <stream/stream.h>
#include <command.h>
#include <playtree.h>
#include <codec-cfg.h>
#include <libmpdemux/demuxer.h>
#include <libmpdemux/stheader.h>
int mp_main(int argc, char *argv[]);
int play_next_file();
int run_playback();
int goto_next_file();
struct mp_cmd *mp_input_parse_cmd(char *str);
int mp_input_queue_cmd(struct input_ctx *ictx, struct mp_cmd *cmd);
void quit_player(MPContext *mpctx, exit_reason how);
struct vo *vo_cmplayer = nullptr;
int mpctx_process_mp_cmds(struct MPContext *mpctx);
int mpctx_playloop(struct MPContext *mpctx);
int mpctx_cleanup_playback(struct MPContext *mpctx);
int mpctx_prepare_playback(struct MPContext *mpctx);
int mpctx_load_file(struct MPContext *mpctx);
int mpctx_init(struct MPContext *mpctx, int argc, char **argv);
int run_playloop(struct MPContext *mpctx);
void mpctx_delete(struct MPContext *mpctx);
void mpctx_new(struct MPContext *mpctx);
int mpctx_idle_loop(struct MPContext *mpctx);
void (*mpctx_paused_changed)(MPContext *) = nullptr;
int (*mpctx_run_cmplayer_cmd)(struct MPContext *mpctx, mp_cmd_t *mpcmd) = nullptr;
mp_cmd *(*mpctx_wait_cmd)(MPContext *ctx, int timeout, int peek) = nullptr;
void mixer_setvolume2(mixer_t *mixer, float l, float r);
extern char *dvd_device;
extern int frame_dropping;

static void checkvolume(struct mixer *mixer)
{
	if (!mixer->ao)
		return;

	ao_control_vol_t vol;
	if (mixer->softvol || CONTROL_OK != ao_control(mixer->ao, AOCONTROL_GET_VOLUME, &vol)) {
		mixer->softvol = true;
		if (!mixer->afilter)
			return;
		float db_vals[AF_NCH];
		if (!af_control_any_rev(mixer->afilter,
						AF_CONTROL_VOLUME_LEVEL | AF_CONTROL_GET, db_vals))
			db_vals[0] = db_vals[1] = 1.0;
		else
			af_from_dB(2, db_vals, db_vals, 20.0, -200.0, 60.0);
		vol.left = (db_vals[0] / (mixer->softvol_max / 100.0)) * 100.0;
		vol.right = (db_vals[1] / (mixer->softvol_max / 100.0)) * 100.0;
	}
	float l = mixer->vol_l;
	float r = mixer->vol_r;
	if (mixer->muted_using_volume)
		l = r = 0;
	/* Try to detect cases where the volume has been changed by some external
	 * action (such as something else changing a shared system-wide volume).
	 * We don't test for exact equality, as some AOs may round the value
	 * we last set to some nearby supported value. 3 has been the default
	 * volume step for increase/decrease keys, and is apparently big enough
	 * to step to the next possible value in most setups.
	 */
	if (FFABS(vol.left - l) >= 3 || FFABS(vol.right - r) >= 3) {
		mixer->vol_l = vol.left;
		mixer->vol_r = vol.right;
		if (mixer->muted_using_volume)
			mixer->muted = false;
	}
	if (!mixer->softvol)
		// Rely on the value not changing if the query is not supported
		ao_control(mixer->ao, AOCONTROL_GET_MUTE, &mixer->muted);
	mixer->muted_by_us &= mixer->muted;
	mixer->muted_using_volume &= mixer->muted;
}

static void setvolume_internal(mixer_t *mixer, float l, float r)
{
	struct ao_control_vol vol = {.left = l, .right = r};
	if (!mixer->softvol) {
		// relies on the driver data being permanent (so ptr stays valid)
		mixer->restore_volume = mixer->ao->no_persistent_volume ?
			mixer->ao->driver->info->short_name : NULL;
		if (ao_control(mixer->ao, AOCONTROL_SET_VOLUME, &vol) != CONTROL_OK)
			mp_tmsg(MSGT_GLOBAL, MSGL_ERR,
					"[Mixer] Failed to change audio output volume.\n");
		return;
	}
	mixer->restore_volume = "softvol";
	if (!mixer->afilter)
		return;
	// af_volume uses values in dB
	float db_vals[AF_NCH];
	int i;
	db_vals[0] = (l / 100.0) * (mixer->softvol_max / 100.0);
	db_vals[1] = (r / 100.0) * (mixer->softvol_max / 100.0);
	for (i = 2; i < AF_NCH; i++)
		db_vals[i] = ((l + r) / 100.0) * (mixer->softvol_max / 100.0) / 2.0;
	af_to_dB(AF_NCH, db_vals, db_vals, 20.0);
	if (!af_control_any_rev(mixer->afilter,
							AF_CONTROL_VOLUME_LEVEL | AF_CONTROL_SET,
							db_vals)) {
		mp_tmsg(MSGT_GLOBAL, MSGL_INFO,
				"[Mixer] No hardware mixing, inserting volume filter.\n");
		if (!(af_add(mixer->afilter, (char*)"volume")
			  && af_control_any_rev(mixer->afilter,
									AF_CONTROL_VOLUME_LEVEL | AF_CONTROL_SET,
									db_vals)))
			mp_tmsg(MSGT_GLOBAL, MSGL_ERR,
					"[Mixer] No volume control available.\n");
	}
}

void mixer_setvolume2(mixer_t *mixer, float l, float r) {
	checkvolume(mixer);  // to check mute status and AO support for volume
	mixer->vol_l = qBound(0.0f, l, 100.0f);//av_clip(l, 0, 100);
	mixer->vol_r = qBound(0.0f, r, 100.0f);//av_clip(r, 0, 100);
	if (!mixer->ao || mixer->muted)
		return;
	setvolume_internal(mixer, mixer->vol_l, mixer->vol_r);
}

}

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

struct PlayEngine::Data {
	Mrl playingMrl;
	QByteArray fileName, nextFileName;
	QTimer ticker = {};
	bool quit = false, playing = false, idling = false, init = false;
	int initSeek = 0, start = 0;

	PlayEngine::Context *ctx = nullptr;
	MPContext *mpctx = nullptr;
	VideoOutput *video = nullptr;
	QMutex q_mutex;		QWaitCondition q_wait;

	double videoAspect = 0.0;
};

PlayEngine::PlayEngine()
: d(new Data) {
	d->video = new VideoOutput(this);
	mpctx_paused_changed = onPausedChanged;
	mpctx_run_cmplayer_cmd = runCmd;
	mpctx_wait_cmd = waitCmd;
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
//	Skin::plug(renderer, &VideoRendererItem::formatChanged, this, &PlayEngine::videoFormatChanged);
}

PlayEngine::~PlayEngine() {
	delete d->video;
	delete d;
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

void PlayEngine::updateVideoAspect(double ratio) {
	d->videoAspect = ratio;
	if (m_renderer)
		m_renderer->setVideoAspectRaito(ratio);
}

void PlayEngine::clear() {
	m_videoStreams.clear();
	updateVideoAspect(0.0);
	m_dvd.clear();
	m_subtitleStreams.clear();
	m_audiosStreams.clear();
	m_title = 0;
	m_isInDvdMenu = false;
}

bool PlayEngine::parse(const Id &id) {
	if (getStream(id, "AUDIO", "AID", m_audiosStreams))
		return true;
	if (getStream(id, "VIDEO", "VID", m_videoStreams))
		return true;
	if (getStream(id, "SUBTITLE", "SID", m_subtitleStreams))
		return true;
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
				updateVideoAspect(id.value.toDouble());
			} else
				return false;
			return true;
		}
//		static QRegExp rxTitle("DVD_TITLE_(\\d+)_(LENGTH|CHAPTERS)");
	}
	return false;
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
		m_isInDvdMenu = false;
	} else if (same(line, "DVDNAV_TITLE_IS_MENU")) {
		m_isInDvdMenu = true;
	} else if (line.startsWith(_L("DVDNAV, switched to title: "))) {
		m_title = line.mid(27).toInt();
	} else
		return false;
	return true;
}



MPContext *PlayEngine::context() const {
	return d->mpctx;
}



mp_cmd *PlayEngine::waitCmd(MPContext *mpctx, int timeout, int peek) {
	Data *d = reinterpret_cast<Context*>(mpctx)->p->d;
	mp_cmd *cmd = mp_input_get_cmd(mpctx->input, 0, peek);
	if (cmd || timeout <= 0)
		return cmd;
	d->q_mutex.lock();
	d->q_wait.wait(&d->q_mutex, timeout);
	cmd = mp_input_get_cmd(mpctx->input, 0, peek);
	d->q_mutex.unlock();
	return cmd;
}

int PlayEngine::runCmd(MPContext *mpctx, mp_cmd_t *mpcmd) {
//	qDebug() << mpcmd->id << mpcmd->args[0].type;
	if (!mpcmd || mpcmd->id != -1)
		return 0;
	PlayEngine *engine = reinterpret_cast<Context*>(mpctx)->p;
	switch (mpcmd->args[0].type) {
	case Cmd::Load:
		return Cmd::Load;
	case Cmd::Quit:
		engine->d->quit = true;
		return Cmd::Quit;
	case Cmd::Stop:
		return Cmd::Stop;
	case Cmd::Volume:
		mixer_setvolume2(&mpctx->mixer, mpcmd->args[0].v.f, mpcmd->args[0].v.f);
		return Cmd::Volume;
	default:
		break;
	}
	return 0;
}

void PlayEngine::enqueue(Cmd *cmd) {
	if (d->mpctx) {
		mp_cmd_t *mpcmd = talloc_ptrtype(NULL, mpcmd);
		mpcmd->id = -1;
		mpcmd->args[0].type = cmd->type;
		if (cmd->type == Cmd::Volume)
			mpcmd->args[0].v.f = cmd->var.toDouble();
		delete cmd;
		mp_input_queue_cmd(d->mpctx->input, mpcmd);
	} else
		delete cmd;
	d->q_wait.wakeAll();
}

void PlayEngine::run_mp_cmd(const char *str) {
	QByteArray tmp = str;
	mp_cmd_t *cmd = mp_input_parse_cmd(tmp.data());
	run_command(d->mpctx, cmd);
	mp_cmd_free(cmd);
}

int PlayEngine::idle() {
	static const double WAKEUP_PERIOD = 0.5;
	while (!d->quit) {
		int ret = -1;
		mp_cmd *mpcmd = waitCmd(d->mpctx, WAKEUP_PERIOD*1000, false);
		if (!mpcmd)
			continue;
		switch (mpcmd->id) {
		case MP_CMD_GET_PROPERTY:
		case MP_CMD_SET_PROPERTY:
		case MP_CMD_STEP_PROPERTY:
			run_command(d->mpctx, mpcmd);
			break;
		case MP_CMD_QUIT:
			ret = Cmd::Quit;
			break;
		case MP_CMD_LOADFILE:
			d->nextFileName = mpcmd->args[0].v.s;
			ret = Cmd::Load;
			break;
		case -1: {
			switch (mpcmd->args[0].type) {
			case Cmd::Load:
				ret = Cmd::Load;
				break;
			case Cmd::Quit:
				ret = Cmd::Quit;
				break;
			default:
				break;
			}
		} default:
			break;
		}
		mp_cmd_free(mpcmd);
		if (ret != -1)
			return ret;
	}
	return Cmd::Quit;
}

bool PlayEngine::isInitialized() const {
	return d->init;
}

void PlayEngine::run() {
	QList<QByteArray> args;
	args << "cmplayer-mplayer2" << "-nofs" << "-fixed-vo" << "-softvol" << "-softvol-max" << "1000.0"
		<< "-noautosub" << "-osdlevel" << "0" << "-quiet" << "-idle" << "-identify"
	<< "-input" << "nodefault-bindings" << "-noconsolecontrols" << "-nomouseinput" << "-noconfig" << "all";
	const int argc = args.size();
	PtrDel<char*, PtrArray> argv(new char*[argc]);
	for (int i=0; i<argc; ++i)
		argv[i] = args[i].data();
	d->ctx = reinterpret_cast<Context*>(talloc_named_const(0, sizeof(*d->ctx), "MPContext"));
	d->ctx->p = this;
	d->mpctx = &d->ctx->mp;
	mpctx_new(d->mpctx);
	mpctx_init(d->mpctx, argc, argv.get());
	vo_cmplayer = d->video->vo_create(d->mpctx);
	d->init = true;
	emit initialized();

	d->quit = false;

	auto getFileName = [this] () -> Cmd::Type {
		if (d->fileName.isEmpty()) {
			int cmd = idle();
			if (cmd == Cmd::Quit) {
				d->quit = true;
				return Cmd::Quit;
			} else if (cmd == Cmd::Load) {
				d->fileName = d->nextFileName;
				d->nextFileName.clear();
				return Cmd::Load;
			} else
				Q_ASSERT_X(false, "PlayEngine::run()", "Invalid command is returned from idle state.");
		}
		return Cmd::Load;
	};

	auto loadOpenFile = [this]() -> MpError {
		const MpError error = (MpError)mpctx_load_file(d->mpctx);
		switch (error) {
		case MpError::None:
			break;
		case MpError::CannotOpenStream:
		case MpError::NoStream:
			qDebug() << "stream error!" << error;
			return error;
		default:
			qDebug() << "unhandled error" << error;
			return error;
		}
		return MpError::None;
	};

	auto play = [this] () -> MpError {
		MpError error = MpError::None;
		d->playing = true;
		d->playingMrl = m_mrl;
		if (d->mpctx->paused)
			run_mp_cmd("pause");
		setState(EnginePlaying);
		emit started(m_mrl);
		int res = Cmd::Unknown;
//		bool volHack = true;
		while (d->mpctx->stop_play == KEEP_PLAYING && !d->quit) {
			error = (MpError)run_playloop(d->mpctx);
			if (error != MpError::None)
				break;
			// -1: quit 0: no seek 1: seek done
			res = mpctx_process_mp_cmds(d->mpctx);
			if (res & Cmd::Break)
				break;
//			if (Q_UNLIKELY(volHack && m_audio)) {
//				const float volume = mpVolume();
//				mixer_setvolume2(&d->mpctx->mixer, volume, volume);
//				volHack = false;
//			}
		}
		if (res & Cmd::Break) {
			setState(EngineStopped);
			emit stopped(d->playingMrl, position(), duration());
			run_mp_cmd("stop");
			if (res == Cmd::Quit)
				d->quit = true;
		} else {
			if (d->mpctx->stop_play == AT_END_OF_FILE) {
				setState(EngineFinished);
				emit finished(d->playingMrl);
			} else
				setState(EngineError);
		}
		d->playing = false;
		return error;
	};

	while (true) {
		d->idling = true;
		const Cmd::Type idle = getFileName();
		d->idling = false;
		if (idle == Cmd::Quit)
			break;
		setState(EngineBuffering);
		m_isInDvdMenu = false;
		if (idle == Cmd::Load) {
			play_tree_t *entry = play_tree_new();
			play_tree_add_file(entry, d->fileName.data());
			if (d->mpctx->playtree)
				play_tree_free_list(d->mpctx->playtree->child, true);
			else
				d->mpctx->playtree = play_tree_new();
			play_tree_set_child(d->mpctx->playtree, entry);

			d->mpctx->playtree_iter = play_tree_iter_new(d->mpctx->playtree, d->mpctx->mconfig);
			if (play_tree_iter_step(d->mpctx->playtree_iter, 0, 0) != PLAY_TREE_ITER_ENTRY) {
				play_tree_iter_free(d->mpctx->playtree_iter);
				d->mpctx->playtree_iter = nullptr;
				setState(EngineError);
				continue;
			}
			d->mpctx->filename = play_tree_iter_get_file(d->mpctx->playtree_iter, 1);
		}
		d->mpctx->filename = d->fileName.data();

		clear();
		emit aboutToOpen();

		MpError error = loadOpenFile();
		if (error == MpError::None) {
			tellmp("speed_set", m_speed);
			if (d->initSeek > 0)
				d->mpctx->opts.seek_to_sec = (double)d->initSeek/1000.0;
			emit seekableChanged(isSeekable());

			auto mixer = &d->mpctx->mixer;
	//		mixer_setvolume2(mixer, 0.0f, 0.0f); mplayer bug? when volume < 1.0, no sound output
			mixer_setmute(mixer, m_muted);
			if (!m_af.isEmpty())
				tellmp("af_add", m_af);

			emit aboutToPlay();
			mpctx_prepare_playback(d->mpctx);
			if (play() != MpError::None)
				setState(EngineError);
		} else
			setState(EngineError);
		if (d->quit && d->mpctx->stop_play == KEEP_PLAYING)
			setState(EngineStopped);
		mpctx_cleanup_playback(d->mpctx);
		d->fileName = d->nextFileName;
		d->nextFileName.clear();
		if (d->quit) {
			qDebug() << "quit!";
			break;
		}
	}
	qDebug() << "finalization started";
	mpctx_delete(d->mpctx);
	d->mpctx = nullptr;
	qDebug() << "mpctx deleted";
	vo_cmplayer = nullptr;
	emit finalized();
}

void PlayEngine::tellmp(const QString &cmd) {
	if (d->mpctx && d->mpctx->input)
		mp_input_queue_cmd(d->mpctx->input, mp_input_parse_cmd(cmd.toLocal8Bit().data()));
}

void PlayEngine::quitRunning() {
	enqueue(new Cmd(Cmd::Quit));
	d->quit = true;
}

void PlayEngine::play(int time) {
	d->initSeek = time;
	d->nextFileName = m_mrl.toString().toLocal8Bit();
	enqueue(new Cmd(Cmd::Load));
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

int PlayEngine::currentSubtitleStream() const {
	return d->mpctx->global_sub_pos;
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
		updateVideoAspect(d->videoAspect);
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
