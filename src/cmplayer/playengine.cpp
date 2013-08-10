#include "playengine.hpp"
#include "info.hpp"
#include "videoframe.hpp"
#include "videooutput.hpp"
#include "hwacc.hpp"
#include "audiocontroller.hpp"
#include "playlistmodel.hpp"
#include "dataevent.hpp"
#include <mpvcore/mp_cmplayer.h>

extern "C" {
#include <mpvcore/command.h>
#include <video/out/vo.h>
#include <video/decode/vd.h>
#include <mpvcore/playlist.h>
#include <mpvcore/codecs.h>
#include <mpvcore/m_property.h>
#include <mpvcore/input/input.h>
#include <audio/filter/af.h>
#include <stream/stream.h>
}

enum EventType {
	UserType = QEvent::User, StreamOpen, UpdateTrack, StateChange, MrlStopped, MrlFinished, PlaylistFinished, MrlChanged, VideoFormatChanged, UpdateChapterList
};

enum MpCmd {MpSetProperty = -1, MpResetAudioChain = -2};

template<typename T> static inline T &getCmdArg(mp_cmd *cmd, int idx = 0);
template<> inline double&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.d;}
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
	MPContext *mpctx = nullptr;
	VideoOutput *video = nullptr;
	GetStartTime getStartTimeFunc;
	PlaylistModel playlist;
	QByteArray hwAccCodecs;
	QMutex imgMutex;
	QMap<QString, QString> subtitleNames;
	QList<QTemporaryFile*> subtitleFiles;
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

	VideoFormat videoFormat;
};

PlayEngine::PlayEngine()
: d(new Data(this)) {
	d->audio = new AudioController(this);
	d->video = new VideoOutput(this);
	mp_register_player_paused_changed(mpPausedChanged);
	mp_register_player_command_filter(mpCommandFilter);
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

	connect(d->video, &VideoOutput::formatChanged, [this] (const VideoFormat &format) {
		post(this, VideoFormatChanged, format);
	});
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

void PlayEngine::setmp(const char *name, double value) {
	d->enqueue<double>(MpSetProperty, name, value);
}

void PlayEngine::setmp(const char *name, int value) {
	d->enqueue<int>(MpSetProperty, name, value);
}

void PlayEngine::setmp(const char *name, float value) {
	d->enqueue<float>(MpSetProperty, name, value);
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

void PlayEngine::setSubtitleStreamsVisible(bool visible) {
	m_subtitleStreamsVisible = visible;
	const auto id = currentSubtitleStream();
	setmp("sub-visibility", (m_subtitleStreamsVisible && id >= 0));
}

void PlayEngine::setCurrentSubtitleStream(int id) {
	setmp("sub-visibility", (m_subtitleStreamsVisible && id >= 0));
	setmp("sub", id);
}

int PlayEngine::currentSubtitleStream() const {
	return currentTrackId(STREAM_SUB);
}

void PlayEngine::addSubtitleStream(const QString &fileName, const QString &enc) {
	QFileInfo info(fileName);
	QFile in(fileName);
	QTemporaryFile *out = new QTemporaryFile(QDir::tempPath() % "/XXXXXX_" % info.fileName());
	if (in.open(QFile::ReadOnly) && out->open()) {
		QTextStream sin, sout;
		sin.setDevice(&in);
		sin.setCodec(enc.toLocal8Bit());
		sout.setDevice(out);
		sout.setCodec("UTF-8");
		QString line;
		while (!(line = sin.readLine()).isNull())
			sout << line << endl;
		sout.flush();
	} else {
		delete out;
		out = nullptr;
	}
	if (out) {
		out->close();
		d->subtitleFiles.append(out);
		d->subtitleNames[out->fileName()] = info.fileName();
		tellmp("sub_add", out->fileName());
	}
}

void PlayEngine::removeSubtitleStream(int id) {
	auto it = m_subtitleStreams.find(id);
	if (it != m_subtitleStreams.end()) {
		auto fileName = it->fileName();
		if (!fileName.isEmpty()) {
			d->subtitleNames.remove(fileName);
			for (int i=0; i<d->subtitleFiles.size(); ++i) {
				if (d->subtitleFiles[i]->fileName() == fileName) {
					delete d->subtitleFiles.takeAt(i);
					break;
				}
			}
		}
		tellmp("sub_remove", id);
	}
}

void PlayEngine::clear() {
	m_dvd.clear();
	m_audioStreams.clear();
	m_videoStreams.clear();
	m_subtitleStreams.clear();
	d->subtitleFiles.clear();
	qDeleteAll(d->subtitleFiles);
	d->subtitleFiles.clear();
	emit audioStreamsChanged(m_audioStreams);
	emit videoStreamsChanged(m_videoStreams);
	emit subtitleStreamsChanged(m_subtitleStreams);
	m_title = 0;
}

template<typename T>
static bool _CheckSwap(T &the, T &one) { if (the != one) { the.swap(one); return true; } return false; }

void PlayEngine::customEvent(QEvent *event) {
	switch ((int)event->type()) {
	case UpdateChapterList: {
		ChapterList chapters; get(event, chapters);
		if (_CheckSwap(m_chapters, chapters))
			emit chaptersChanged(m_chapters);
		break;
	} case UpdateTrack: {
		QVector<StreamList> streams;
		get(event, streams);
		if (_CheckSwap(m_videoStreams, streams[STREAM_VIDEO]))
			emit videoStreamsChanged(m_videoStreams);
		if (_CheckSwap(m_audioStreams, streams[STREAM_AUDIO]))
			emit audioStreamsChanged(m_audioStreams);
		if (!streams[STREAM_SUB].isEmpty()) {
			streams[STREAM_SUB][-1].m_name = tr("No Subtitle");
			for (auto &one : streams[STREAM_SUB]) {
				auto it = d->subtitleNames.constFind(one.m_title);
				if (it != d->subtitleNames.constEnd()) {
					one.m_fileName.swap(one.m_title);
					one.m_title = *it;
					qDebug() << one.m_fileName;
				}
			}
		}
		if (_CheckSwap(m_subtitleStreams, streams[STREAM_SUB]))
			emit subtitleStreamsChanged(m_subtitleStreams);
		break;
	} case StreamOpen:
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
	} case VideoFormatChanged: {
		VideoFormat format; get(event, format);
		if (_Change(d->videoFormat, format))
			emit videoFormatChanged(d->videoFormat);
	} default:
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
	if (id.name.isEmpty())
		return false;
	if (id.name.startsWith(_L("DVD_"))) {
		auto dvd = id.name.midRef(4);
		if (_Same(dvd, "TITLES")) {
//			m_dvd.titles[id.value.toInt()];
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
		} else if (_Same(dvd, "CURRENT_TITLE")) {
			m_title = id.value.toInt();
		} else
			return false;
		return true;
	} else
		return false;
	return true;
}

MPContext *PlayEngine::context() const {
	return d->mpctx;
}

int PlayEngine::mpCommandFilter(MPContext *mpctx, mp_cmd *cmd) {
	if (cmd->id < 0) {
		switch (cmd->id) {
		case MpSetProperty:
			mp_property_do(cmd->name, M_PROPERTY_SET, &cmd->args[0].v.f, mpctx);
			break;
		case MpResetAudioChain:
			reinit_audio_chain(mpctx);
		default:
			break;
		}
		cmd->id = MP_CMD_IGNORE;
		return true;
	}
	return false;
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

int PlayEngine::playImage(const Mrl &mrl, int &terminated, int &duration) {
	QImage image;
	if (!image.load(mrl.toLocalFile()))
		return MPERROR_OPEN_FILE;
	auto mpctx = d->mpctx;
	auto error = prepare_playback(mpctx);
	if (error != MPERROR_NONE)
		return error;
	setState(mpctx->paused ? EnginePaused : EnginePlaying);
	d->video->output(image);
	m_imgPos = 0;
	post(this, StreamOpen);
	post(this, UpdateChapterList, ChapterList());
	TimeLine time;
	while (!mpctx->stop_play) {
		mp_cmd_t *cmd = nullptr;
		while ((cmd = mp_input_get_cmd(mpctx->input, 0, 1)) != NULL) {
			cmd = mp_input_get_cmd(mpctx->input, 0, 0);
			run_command(mpctx, cmd);
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
	return error;
}

int PlayEngine::playAudioVideo(const Mrl &/*mrl*/, int &terminated, int &duration) {
	d->video->output(QImage());
	auto mpctx = d->mpctx;
	mpctx->opts->video_decoders = d->hwAccCodecs.data();
	d->mpctx->opts->play_start.pos = d->start*1e-3;
	d->mpctx->opts->play_start.type = REL_TIME_ABSOLUTE;
	setmp("speed", m_speed);
	setmp("audio-delay", m_audioSync*0.001);
	auto error = prepare_playback(mpctx);
	QVector<StreamList> streams(STREAM_TYPE_COUNT);
	QString name[STREAM_TYPE_COUNT];
	name[STREAM_AUDIO] = tr("Audio %1");
	name[STREAM_VIDEO] = tr("Video %1");
	name[STREAM_SUB] = tr("Subtitle %1");
	if (error != MPERROR_NONE)
		return error;
	setState(mpctx->paused ? EnginePaused : EnginePlaying);
	post(this, StreamOpen);
	ChapterList chapters(qMax(0, get_chapter_count(mpctx)));
	for (int i=0; i<chapters.size(); ++i) {
		const QString time = _MSecToString(qRound(chapter_start_time(mpctx, i)*1000), _L("hh:mm:ss.zzz"));
		if (char *name = chapter_name(mpctx, i)) {
			chapters[i].m_name = QString::fromLocal8Bit(name) % '(' % time % ')';
			talloc_free(name);
		} else
			chapters[i].m_name = '(' % QString::number(i+1) % ") " % time;
		chapters[i].m_id = i;
	}
	uint title = 0, titles = 0;
	if (mpctx->demuxer && mpctx->demuxer->stream) {
		auto stream = mpctx->demuxer->stream;
		stream->control(stream, STREAM_CTRL_GET_CURRENT_TITLE, &title);
		stream->control(stream, STREAM_CTRL_GET_NUM_TITLES, &titles);
	}
	post(this, UpdateChapterList, chapters);

	while (!mpctx->stop_play) {
		run_playloop(mpctx);
		if (!mpctx->stop_play && streams[STREAM_AUDIO].size() + streams[STREAM_VIDEO].size() + streams[STREAM_SUB].size() != mpctx->num_tracks) {
			streams[STREAM_AUDIO].clear(); streams[STREAM_VIDEO].clear(); streams[STREAM_SUB].clear();
			for (int i=0; i<mpctx->num_tracks; ++i) {
				const auto track = mpctx->tracks[i];
				auto &list = streams[track->type];
				if (!list.contains(track->user_tid)) {
					auto &stream = list[track->user_tid];
					stream.m_title = QString::fromLocal8Bit(track->title);
					stream.m_lang = QString::fromLocal8Bit(track->lang);
					stream.m_id = track->user_tid;
					stream.m_name = name[track->type].arg(track->user_tid+1);
				}
			}
			post(this, UpdateTrack, streams);
		}
	}
	terminated = position();
	duration = this->duration();
	return error;
}

void PlayEngine::setMpVolume() {
	d->audio->setVolume(m_muted ? 0.0 : qBound(0.0, m_preamp*m_volume*0.01, 10.0));
}

void PlayEngine::run() {
	CharArrayList args = QStringList()
		<< "cmplayer-mpv" << "--no-config" << "--idle" << "--no-fs"
		<< ("--af=dummy=" % QString::number((quint64)(quintptr)(void*)(d->audio)))
		<< ("--vo=null:address=" % QString::number((quint64)(quintptr)(void*)(d->video)))
		<< "--fixed-vo" << "--no-autosub" << "--osd-level=0" << "--quiet" << "--identify"
		<< "--no-consolecontrols" << "--no-mouseinput" << "--subcp=utf8";
	auto mpctx = d->mpctx = create_player(args.size(), args.data());
	Q_ASSERT(d->mpctx);
	d->mpctx->priv = this;
	d->init = true;
	HwAcc hwAcc; (void)hwAcc;
	d->quit = false;
	while (!d->quit) {
		m_imgMode = false;
		idle_player(mpctx);
		if (mpctx->stop_play == PT_QUIT)
			break;
		Q_ASSERT(mpctx->playlist->current);
		clear();
		int terminated = 0, duration = 0;
		Mrl mrl = d->playlist.loadedMrl();
		d->playing = true;
		setState(EngineBuffering);
		m_imgMode = mrl.isImage();
		int error = m_imgMode ? playImage(mrl, terminated, duration) : playAudioVideo(mrl, terminated, duration);
		clean_up_playback(mpctx);
		if (error != MPERROR_NONE)
			setState(EngineError);
		d->playing = false;
		qDebug() << "terminate playback";
		if (mpctx->stop_play == PT_QUIT) {
			if (error == MPERROR_NONE) {
				setState(EngineStopped);
				post(this, MrlStopped, d->playlist.loadedMrl(), terminated, duration);
			}
			break;
		}
		playlist_entry *entry = nullptr;
		if (error == MPERROR_NONE) {
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
		}
		mpctx->playlist->current = entry;
		mpctx->playlist->current_was_replaced = false;
		mpctx->stop_play = KEEP_PLAYING;
		if (!mpctx->playlist->current && !mpctx->opts->player_idle_mode)
			break;
	}
	qDebug() << "terminate loop";
	d->video->quit();
	mpctx->opts->video_decoders = nullptr;
	destroy_player(mpctx);
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

void PlayEngine::mpPausedChanged(MPContext *mpctx, int paused) {
	auto engine = reinterpret_cast<PlayEngine*>(mpctx->priv);
	if (mpctx->stop_play == KEEP_PLAYING)
		engine->setState(paused ? EnginePaused : EnginePlaying);
}

void PlayEngine::pause() {
	if (m_imgMode)
		setState(EnginePaused);
	else
		setmp("pause", 1);
}

void PlayEngine::unpause() {
	if (m_imgMode)
		setState(EnginePlaying);
	else
		setmp("pause", 0);
}

void PlayEngine::play() {
	switch (m_state) {
	case EngineStopped:
	case EngineFinished:
		play(d->getStartTime(d->playlist.loadedMrl()));
		break;
	default:
		unpause();
		break;
	}
}

void PlayEngine::setPlaylist(const Playlist &playlist) {
	d->playlist.setPlaylist(playlist);
}

Mrl PlayEngine::mrl() const {
	return d->playlist.loadedMrl();
}

int PlayEngine::currentTrackId(int type) const {
	return (d->mpctx && d->mpctx->current_track[type]) ? d->mpctx->current_track[type]->user_tid : -1;
}

int PlayEngine::currentAudioStream() const {
	return currentTrackId(STREAM_AUDIO);
}

int PlayEngine::currentVideoStream() const {
	return hasVideo() ? currentTrackId(STREAM_VIDEO) : -1;
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
	return d->videoFormat;
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

void PlayEngine::setVolumeNormalizer(double length, double target, double silence, double min, double max) {
	d->audio->setNormalizer(length, target, silence, min, max);
}
