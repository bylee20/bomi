#include "playengine.hpp"
#include <QtCore/QDebug>
#include "avmisc.hpp"
#include "recentinfo.hpp"
#include "pref.hpp"
#include "videooutput.hpp"
#include "videorenderer.hpp"
#include "audiocontroller.hpp"
#include <QtGui/QMessageBox>
#include <libmpdemux/stheader.h>
#include <QtCore/QStringBuilder>
#include <QtGui/QApplication>
#include <QtCore/QVariant>
#include <QtCore/QTimer>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include "mpcore.hpp"
#include <QtCore/QFileInfo>

enum MpError {None = 0, UserInterrupted, CannotOpenStream, NoStream, InitVideoFilter, InitAudioFilter};
extern "C" {
#include <input/input.h>
#include <command.h>
#include <playtree.h>
#include <libmpdemux/demuxer.h>
int mp_main(int argc, char *argv[]);
int play_next_file();
int run_playback();
int goto_next_file();
int mp_input_parse_and_queue_cmds(struct input_ctx *ictx, const char *str);
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
int (*mpctx_update_video)(struct MPContext *mpctx) = nullptr;
mp_cmd *(*mpctx_wait_cmd)(MPContext *ctx, int timeout, int peek) = nullptr;
void mixer_setvolume2(mixer_t *mixer, float l, float r);
extern char *dvd_device;
}

PlayEngine *PlayEngine::obj = nullptr;

struct PlayEngine::Context {
	MPContext mp;
	PlayEngine *p;
};

struct PlayEngine::Data {
	Mrl mrl, playingMrl;
	QByteArray filename, next_filename;
	QTimer ticker;		VideoFormat vfmt;
	State state = State::Stopped;
	bool quit = false, playing = false, isMenu = false;
	bool idling = false, videoUpdate = false;
	int duration = 0, initSeek = 0, title = 0;
	double speed = 1.0;
	Context *ctx = nullptr;
	MPContext *mpctx = nullptr;
	VideoOutput *video = nullptr;
	StreamList spus;
	DvdInfo dvd;
	QMutex q_mutex;
	QWaitCondition q_wait;
	QByteArray dvdDevice;
};

PlayEngine::PlayEngine()
: d(new Data) {
	d->video = new VideoOutput(this);
	mpctx_paused_changed = onPausedChanged;
	mpctx_run_cmplayer_cmd = runCmd;
	mpctx_update_video = updateVideo;
	mpctx_wait_cmd = waitCmd;
	connect(&d->ticker, SIGNAL(timeout()), this, SLOT(emitTick()));
	d->ticker.setInterval(100);
	d->ticker.start();
}

PlayEngine::~PlayEngine() {
	delete d->video;
	delete d;
}

int PlayEngine::updateVideo(struct MPContext *mpctx) {
	Data *d = reinterpret_cast<Context*>(mpctx)->p->d;
	if (d->video->renderer()->needsToBeRendered()) {
		d->video->renderer()->render();
		return true;
	} else
		return false;
}

bool PlayEngine::isMenu() const {
	return d->isMenu;
}

void PlayEngine::setDvdDevice(const QString &name) {
	d->dvdDevice = name.toLocal8Bit();
//	dvd_device = d->dvdDevice.data();
}

void PlayEngine::clear() {
	d->dvd.clear();
	d->spus.clear();
	d->title = 0;
	d->isMenu = false;
}

bool PlayEngine::parse(const Id &id) {
	if (getStream(id, "SUBTITLE", "SID", d->spus))
		return true;
	else if (id.name.startsWith(_L("DVD_"))) {
		if (same(id.name, "DVD_VOLUME_ID")) {
			d->dvd.volume = id.value;
			d->dvd.titles[0].name = tr("DVD Menu");
			return true;
		} else if (same(id.name, "DVD_CURRENT_TITLE")) {
			d->title = id.value.toInt();
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
		auto &title = d->dvd.titles[idx];
		title.name = tr("Title %1").arg(idx);
		title.chapters.clear();
		title.chapters << "00:00:00";
		title.chapters += rxTitle.cap(2).split(',', QString::SkipEmptyParts);
		title.chapters.pop_back();
		return true;
	} else if (same(line, "DVDNAV_TITLE_IS_MOVIE")) {
		d->isMenu = false;
		return true;
	} else if (same(line, "DVDNAV_TITLE_IS_MENU")) {
		d->isMenu = true;
		return true;
	} else if (line.startsWith(_L("DVDNAV, switched to title: "))) {
		d->title = line.mid(27).toInt();
		return true;
	}
	return false;
}

void PlayEngine::emitTick() {
	if (!d->mpctx)
		return;
	switch (d->state) {
	case State::Playing:
	case State::Paused: {
		const int duration = qRound(get_time_length(d->mpctx)*1000.0);
		if (d->duration != duration)
			emit durationChanged(d->duration = duration);
		emit tick(position());
		break;
	} default:
		break;
	}
}

MPContext *PlayEngine::context() const {
	return d->mpctx;
}

VideoRenderer &PlayEngine::renderer() const {
	return *d->video->renderer();
}

void PlayEngine::updateState(State state) {
	if (d->state == state)
		return;
	const State old = d->state;
	d->state = state;
	emit stateChanged(d->state, old);
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
	case Cmd::VideoUpdate:
		engine->d->videoUpdate = true;
		break;
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
	mp_cmd_t *mpcmd = talloc_ptrtype(NULL, mpcmd);
	mpcmd->id = -1;
	mpcmd->args[0].type = cmd->type;
	if (cmd->type == Cmd::Volume)
		mpcmd->args[0].v.f = cmd->var.toDouble();
	delete cmd;
	mp_input_queue_cmd(d->mpctx->input, mpcmd);
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
			d->next_filename = mpcmd->args[0].v.s;
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
			case Cmd::VideoUpdate:
				for (;;) {
					auto cmd = mp_input_get_cmd(d->mpctx->input, 0, true);
					if (!cmd || cmd->id != -1 || cmd->args[0].type != Cmd::VideoUpdate)
						break;
					cmd = mp_input_get_cmd(d->mpctx->input, 0, false);
					mp_cmd_free(cmd);
				}
				d->video->renderer()->render();
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

void PlayEngine::run() {
	QList<QByteArray> args;
	args << "cmplayer-mplayer2" << "-nofs" << "-fixed-vo" << "-softvol" << "-softvol-max" << "1000.0"
	<< "-noautosub" << "-osdlevel" << "0" << "-quiet" << "-idle" << "-identify"// << "-framedrop"
		<< "-input" << "nodefault-bindings" << "-noconsolecontrols" << "-nomouseinput";

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
	emit initialized();

	d->quit = false;

	auto getFileName = [this] () -> Cmd::Type {
		if (d->filename.isEmpty()) {
			int cmd = idle();
			if (cmd == Cmd::Quit) {
				d->quit = true;
				return Cmd::Quit;
			} else if (cmd == Cmd::Load) {
				d->filename = d->next_filename;
				d->next_filename.clear();
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
		d->playingMrl = d->mrl;
		if (d->mpctx->paused)
			run_mp_cmd("pause");
		updateState(State::Playing);
		emit started(d->mrl);
		int res = Cmd::Unknown;
		bool volHack = true;
		while (d->mpctx->stop_play == KEEP_PLAYING && !d->quit) {
			error = (MpError)run_playloop(d->mpctx);
			if (error != MpError::None)
				break;
			// -1: quit 0: no seek 1: seek done
			res = mpctx_process_mp_cmds(d->mpctx);
			if (res & Cmd::Break)
				break;
			if (d->videoUpdate && d->mpctx->paused) {
				d->video->renderer()->render();
				d->videoUpdate = false;
			}
			if (Q_UNLIKELY(volHack && m_audio)) {
				const float volume = m_audio->realVolume();
				mixer_setvolume2(&d->mpctx->mixer, volume, volume);
				volHack = false;
			}
		}
		if (res & Cmd::Break) {
			updateState(State::Stopped);
			emit stopped(d->playingMrl, position(), duration());
			run_mp_cmd("stop");
			if (res == Cmd::Quit)
				d->quit = true;
		} else {
			if (d->mpctx->stop_play == AT_END_OF_FILE) {
				updateState(State::Finished);
				emit finished(d->playingMrl);
			} else
				updateState(State::Error);
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
		updateState(State::Buffering);
		d->isMenu = false;
		if (idle == Cmd::Load) {
			play_tree_t *entry = play_tree_new();
			play_tree_add_file(entry, d->filename.data());
			if (d->mpctx->playtree)
				play_tree_free_list(d->mpctx->playtree->child, true);
			else
				d->mpctx->playtree = play_tree_new();
			play_tree_set_child(d->mpctx->playtree, entry);

			d->mpctx->playtree_iter = play_tree_iter_new(d->mpctx->playtree, d->mpctx->mconfig);
			if (play_tree_iter_step(d->mpctx->playtree_iter, 0, 0) != PLAY_TREE_ITER_ENTRY) {
				play_tree_iter_free(d->mpctx->playtree_iter);
				d->mpctx->playtree_iter = nullptr;
				updateState(State::Error);
				continue;
			}
			d->mpctx->filename = play_tree_iter_get_file(d->mpctx->playtree_iter, 1);
		}
		d->mpctx->filename = d->filename.data();

		clear();
		emit aboutToOpen();
		MpError error = loadOpenFile();
		if (error == MpError::None) {
			tellmp("speed_set", d->speed);
			if (d->initSeek > 0)
				d->mpctx->opts.seek_to_sec = (double)d->initSeek/1000.0;
			emit seekableChanged(isSeekable());
			emit aboutToPlay();
			mpctx_prepare_playback(d->mpctx);
			if (play() != MpError::None)
				updateState(State::Error);
		} else
			updateState(State::Error);
		if (d->quit && d->mpctx->stop_play == KEEP_PLAYING)
			updateState(State::Stopped);
		mpctx_cleanup_playback(d->mpctx);
		d->filename = d->next_filename;
		d->next_filename.clear();
		if (d->quit) {
			qDebug() << "quit!";
			break;
		}
	}
	qDebug() << "finalization started";
	mpctx_delete(d->mpctx);
	qDebug() << "mpctx deleted";
	vo_cmplayer = nullptr;
	emit finalized();
}

void PlayEngine::tellmp(const QString &cmd) {
	mp_input_parse_and_queue_cmds(d->mpctx->input, cmd.toLocal8Bit());
}

void PlayEngine::tellmp(const QString &cmd, const QVariant &arg) {
	tellmp(cmd % ' ' % arg.toString());
}

void PlayEngine::tellmp(const QString &cmd, const QVariant &arg1, const QVariant &arg2) {
	tellmp(cmd % ' ' % arg1.toString() % ' ' % arg2.toString());
}

void PlayEngine::tellmp(const QString &cmd, const QVariant &arg1, const QVariant &arg2, const QVariant &arg3) {
	tellmp(cmd % ' ' % arg1.toString() % ' ' % arg2.toString() % ' ' % arg3.toString());
}

void PlayEngine::tellmp(const QString &cmd, const QStringList &args) {
	QString full = cmd;
	for (int i=0; i<args.size(); ++i) {
		full += ' ';
		full += args[i];
	}
	tellmp(full);
}

void PlayEngine::quitRunning() {
	d->quit = true;
	enqueue(new Cmd(Cmd::Quit));
}

void PlayEngine::load() {
	d->initSeek = getStartTime();
	d->next_filename = d->mrl.toString().toLocal8Bit();
	enqueue(new Cmd(Cmd::Load));
}

void PlayEngine::setMrl(const Mrl &mrl, bool play) {
	if (mrl != d->mrl) {
		d->mrl = mrl;
		emit mrlChanged(d->mrl);
		if (play)
			load();
	}
}

int PlayEngine::position() const {
	return d->mpctx && d->mpctx->demuxer ? get_current_time(d->mpctx)*1000.0 + 0.5 : 0;
}

bool PlayEngine::isSeekable() const {
	return d->mpctx && d->mpctx->demuxer && d->mpctx->demuxer->seekable;
}

void PlayEngine::setSpeed(double speed) {
	if (d->speed != speed) {
		d->speed = speed;
		if (d->playing)
			tellmp("speed_set", speed);
	}
}

bool PlayEngine::hasVideo() const {
	return d->mpctx && d->mpctx->sh_video;
}

State PlayEngine::state() const {
	return d->state;
}

double PlayEngine::speed() const {
	return d->speed;
}

int PlayEngine::duration() const {
	return d->duration;
}

bool PlayEngine::atEnd() const {
	return d->mpctx->stop_play == AT_END_OF_FILE;
}

Mrl PlayEngine::mrl() const {
	return d->mrl;
}

int PlayEngine::currentTitleId() const {
	return d->isMenu ? 0 : d->title;
}

int PlayEngine::currentChapterId() const {
	if (d->playing)
		return get_current_chapter(d->mpctx);
	return -2;
}

int PlayEngine::currentSpuId() const {
	return d->mpctx->global_sub_pos;
}

QStringList PlayEngine::chapters() const {
	auto it = d->dvd.titles.find(currentTitleId());
	return it != d->dvd.titles.end() ? it->chapters : QStringList();
}

QMap<int, DvdInfo::Title> PlayEngine::titles() const {
	return d->dvd.titles;
}

StreamList PlayEngine::spus() const {
	return d->spus;
}

void PlayEngine::setCurrentTitle(int id) {
	if (id > 0)
		tellmp("switch_title", id);
	else
		tellmp("dvdnav menu");
}

void PlayEngine::setCurrentChapter(int id) {
	tellmp("seek_chapter", id, 1);
}

void PlayEngine::setCurrentSpu(int id) {
	tellmp("sub_demux", id);
}

void PlayEngine::onPausedChanged(MPContext *mpctx) {
	PlayEngine *engine = reinterpret_cast<Context*>(mpctx)->p;
	if (mpctx->stop_play == KEEP_PLAYING)
		engine->updateState(mpctx->paused ? State::Paused : State::Playing);
}

int PlayEngine::getStartTime() const {
	const RecentInfo &recent = RecentInfo::get();
	const int seek = recent.stoppedTime(d->mrl);
	if (seek <= 0)
		return 0;
	if (Pref::get().ask_record_found) {
		const QDateTime date = recent.stoppedDate(d->mrl);
		const QString title = tr("Stopped Record Found");
		const QString text = tr("This file was stopped during its playing before.\n"
			"Played Date: %1\nStopped Time: %2\n"
			"Do you want to start from where it's stopped?\n"
			"(You can configure not to ask anymore in the preferecences.)")
			.arg(date.toString(Qt::ISODate)).arg(msecToString(seek, "h:mm:ss"));
		const QMessageBox::StandardButtons b = QMessageBox::Yes | QMessageBox::No;
		if (QMessageBox::question(QApplication::activeWindow(), title, text, b) != QMessageBox::Yes)
			return 0;
	}
	return seek;
}

void PlayEngine::play() {
	switch (d->state) {
	case State::Paused:
		tellmp("pause");
		break;
	case State::Stopped:
	case State::Finished:
		load();
		break;
	default:
		break;
	}
}

void PlayEngine::pause() {
	if (!isPaused()) {
		tellmp("pause");
	}
}

void PlayEngine::stop() {
	enqueue(new Cmd(Cmd::Stop));
}

void PlayEngine::seek(int pos) {
	tellmp("seek", (double)pos/1000.0, 2);
}

QString PlayEngine::mediaName() const {
	if (d->mrl.isLocalFile())
		return tr("File name: ") % QFileInfo(d->mrl.toLocalFile()).fileName();
	if (d->mrl.isDvd() && !d->dvd.volume.isEmpty())
		return _L("DVD: ") % d->dvd.volume;
	return _L("URL: ") % d->mrl.toString();
}
