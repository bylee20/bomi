#include "playengine.hpp"
#include "info.hpp"
#include "videoframe.hpp"
#include "videooutput.hpp"
#include "hwacc.hpp"
#include "audiocontroller.hpp"
#include "playlistmodel.hpp"
#include "deintinfo.hpp"
#include "dataevent.hpp"
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

static inline PlayEngine *priv(MPContext *mpctx) { return static_cast<PlayEngine*>(mpctx->priv); }

template<typename T> static inline T &getCmdArg(mp_cmd *cmd, int idx = 0);
template<> inline double&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.d;}
template<> inline float	&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.f;}
template<> inline int	&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.i;}
template<> inline char*	&getCmdArg(mp_cmd *cmd, int idx) {return cmd->args[idx].v.s;}

extern void initialize_vdpau();
extern void finalize_vdpau();
extern void initialize_vaapi();
extern void finalize_vaapi();

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
	GetMrlInt getStartTimeFunc, getCacheFunc;
	PlaylistModel playlist;
	QByteArray hwAccCodecs;
	QMutex imgMutex;
	QMutex mutex;
	QMap<QString, QString> subtitleNames;
	QList<QTemporaryFile*> subtitleFiles;
	int getStartTime(const Mrl &mrl) { return getStartTimeFunc ? getStartTimeFunc(mrl) : 0; }
	int getCache(const Mrl &mrl) { return getCacheFunc ? getCacheFunc(mrl) : 0; }
	QByteArray &setFileName(const Mrl &mrl) {
		fileName = "\"";
		fileName += mrl.toString().toLocal8Bit();
		fileName += "\"";
		return fileName;
	}
	QByteArray vfs;
	template<typename T = int>
	bool enqueue(int id, const char *name = "", const T &v = 0) {
		const bool ret = mpctx && mpctx->input;
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
	DeintOption deint_swdec, deint_hwdec;
	bool deint = false;
	QByteArray ao = "";
	AudioDriver audioDriver = AudioDriver::Auto;
};

PlayEngine::PlayEngine()
: d(new Data(this)) {
	d->audio = new AudioController(this);
	d->video = new VideoOutput(this);
	mp_register_player_command_filter(mpCommandFilter);
	connect(&d->ticker, &QTimer::timeout, [this] () {
		if (m_imgMode)
			emit tick(m_imgPos);
		else if (d->mpctx && (isPaused() || isPlaying())) {
			const bool start = _Change(m_startPos, qMax(0, qRound(get_start_time(d->mpctx)*1000.0)));
			const bool duration = _Change(m_duration, qRound(get_time_length(d->mpctx)*1000.0));
			if (start)
				emit startPositionChanged(m_startPos);
			if (duration)
				emit durationChanged(m_duration);
			emit tick(position());
		}
	});
	d->ticker.setInterval(20);
	d->ticker.start();

	connect(d->video, &VideoOutput::formatChanged, [this] (const VideoFormat &format) {
		postData(this, VideoFormatChanged, format);
	});
	connect(&d->playlist, &PlaylistModel::playRequested, [this] (int row) {
		load(row, d->getStartTime(d->playlist[row]));
	});
}

PlayEngine::~PlayEngine() {
	delete d->audio;
	delete d->video;
//	finalizeGL();
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

void PlayEngine::setClippingMethod(ClippingMethod method) {
	d->audio->setClippingMethod(method);
}

typedef QPair<AudioDriver, const char*> AudioDriverName;
const std::array<AudioDriverName, AudioDriverInfo::size()-1> audioDriverNames = {{
	{AudioDriver::ALSA, "alsa"},
	{AudioDriver::PulseAudio, "pulse"},
	{AudioDriver::CoreAudio, "coreaudio"},
	{AudioDriver::PortAudio, "portaudio"},
	{AudioDriver::JACK, "jack"},
	{AudioDriver::OpenAL, "openal"}
}};

void PlayEngine::setAudioDriver(AudioDriver driver) {
	if (_Change(d->audioDriver, driver)) {
		auto it = _FindIf(audioDriverNames, [driver] (const AudioDriverName &one) { return one.first == driver; });
		d->ao = it != audioDriverNames.end() ? it->second : "";
	}
}

AudioDriver PlayEngine::preferredAudioDriver() const {
	return d->audioDriver;
}

AudioDriver PlayEngine::audioDriver() const {
	if (!d->mpctx->ao)
		return preferredAudioDriver();
	auto name = d->mpctx->ao->driver->info->short_name;
	auto it = _FindIf(audioDriverNames, [name] (const AudioDriverName &one) { return !qstrcmp(name, one.second);});
	return it != audioDriverNames.end() ? it->first : AudioDriver::Auto;
}

void PlayEngine::setMinimumCache(int playback, int seeking) {
	d->mpctx->opts->stream_cache_min_percent = playback;
	d->mpctx->opts->stream_cache_pause = playback*0.5;
	d->mpctx->opts->stream_cache_seek_min_percent = seeking;
}

void PlayEngine::setGetCacheFunction(const GetMrlInt &func) {
	d->getCacheFunc = func;
}

void PlayEngine::setGetStartTimeFunction(const GetMrlInt &func) {
	d->getStartTimeFunc = func;
}

void PlayEngine::setmp(const char *name, double value) {
	d->enqueue(MpSetProperty, name, value);
}

void PlayEngine::setmp(const char *name, int value) {
	d->enqueue(MpSetProperty, name, value);
}

void PlayEngine::setmp(const char *name, float value) {
	d->enqueue(MpSetProperty, name, value);
}

double PlayEngine::volumeNormalizer() const {
	auto gain = d->audio->gain(); return gain < 0 ? 1.0 : gain;
}

bool PlayEngine::isHwAccActivated() const {
	return d->video->hwAcc() != nullptr;
}

void PlayEngine::setHwAccCodecs(const QList<int> &codecs) {
	d->hwAccCodecs.clear();
	for (auto id : codecs) {
		if (const char *name = HwAcc::codecName(id)) {
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
		auto chapters = getData<ChapterList>(event);
		if (_CheckSwap(m_chapters, chapters))
			emit chaptersChanged(m_chapters);
		break;
	} case UpdateTrack: {
		auto streams = getData<std::array<StreamList, STREAM_TYPE_COUNT>>(event);
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
		const auto state = getData<EngineState>(event);
		if (_Change(m_state, state))
			emit stateChanged(m_state);
		break;
	} case MrlStopped: {
		Mrl mrl; int terminated = 0, duration = 0;
		getAllData(event, mrl, terminated, duration);
		emit stopped(mrl, terminated, duration);
		break;
	} case MrlFinished: {
		const auto mrl = getData<Mrl>(event);
		emit finished(mrl);
		break;
	} case PlaylistFinished:
		emit d->playlist.finished();
		break;
	case MrlChanged: {
		const auto mrl = getData<Mrl>(event);
		emit mrlChanged(mrl);
	} case VideoFormatChanged: {
		const auto format = getData<VideoFormat>(event);
		if (_Change(d->videoFormat, format))
			emit videoFormatChanged(d->videoFormat);
	} default:
		break;
	}
}

void PlayEngine::setState(EngineState state) {
	postData(this, StateChange, state);
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
	auto e = static_cast<PlayEngine*>(mpctx->priv); auto d = e->d;
	if (cmd->id < 0) {
		QMutexLocker locker(&d->mutex);
		switch (cmd->id) {
		case MpSetProperty:
			mp_property_do(cmd->name, M_PROPERTY_SET, &cmd->args[0].v, mpctx);
			break;
		case MpSetAudioLevel:
			d->audio->setLevel(cmd->args[0].v.d);
			break;
		case MpSetAudioMuted:
			d->audio->setMuted(cmd->args[0].v.i);
			break;
		case MpResetAudioChain:
			reinit_audio_chain(mpctx);
			break;
		case MpResetDeint:
			d->video->setDeintOptions(d->deint_swdec, d->deint_hwdec);
			break;
		case MpSetDeintEnabled:
			d->video->setDeintEnabled(d->deint);
			break;
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
	postData(this, StreamOpen);
	postData(this, UpdateChapterList, ChapterList());
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
	if (d->hwAccCodecs.isEmpty()) {
		d->mpctx->opts->hwdec_api = HWDEC_NONE;
		d->mpctx->opts->hwdec_codecs = nullptr;
	} else {
#ifdef Q_OS_LINUX
		d->mpctx->opts->hwdec_api = HWDEC_VAAPI;
#elif defined(Q_OS_MAC)
		d->mpctx->opts->hwdec_api = HWDEC_VDA;
#endif
		d->mpctx->opts->hwdec_codecs = d->hwAccCodecs.data();
	}
	d->mpctx->opts->stream_cache_size = d->getCache(Mrl(QString::fromLocal8Bit(d->mpctx->playlist->current->filename)));
	d->mpctx->opts->audio_driver_list->name = d->ao.data();
	d->mpctx->opts->play_start.pos = d->start*1e-3;
	d->mpctx->opts->play_start.type = REL_TIME_ABSOLUTE;
	setmp("audio-delay", m_audioSync*0.001);
	d->video->setDeintOptions(d->deint_swdec, d->deint_hwdec);
	d->video->setDeintEnabled(d->deint);
	auto error = prepare_playback(mpctx);
	updateAudioLevel();
	std::array<StreamList, STREAM_TYPE_COUNT> streams;
	QString name[STREAM_TYPE_COUNT];
	name[STREAM_AUDIO] = tr("Audio %1");
	name[STREAM_VIDEO] = tr("Video %1");
	name[STREAM_SUB] = tr("Subtitle %1");
	if (error != MPERROR_NONE)
		return error;
	postData(this, StreamOpen);
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
	postData(this, UpdateChapterList, chapters);
	auto prevState = EngineLoading;
	tellmp("vf set", d->vfs);
	while (!mpctx->stop_play) {
		run_playloop(mpctx);
		if (mpctx->stop_play)
			break;
		auto state = prevState;
		if (mpctx->paused_for_cache && !mpctx->opts->pause)
			state = EngineLoading;
		else if (mpctx->paused)
			state = EnginePaused;
		else
			state = EnginePlaying;
		if (_Change(prevState, state))
			setState(state);
		if (streams[STREAM_AUDIO].size() + streams[STREAM_VIDEO].size() + streams[STREAM_SUB].size() != mpctx->num_tracks) {
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
			postData(this, UpdateTrack, streams);
		}
	}
	terminated = position();
	duration = this->duration();
	return error;
}

void PlayEngine::updateAudioLevel() {
	d->enqueue(MpSetAudioLevel, "", (double)(m_volume)*0.01*m_preamp);
}

void PlayEngine::setVolume(int volume) {
	if (_Change(m_volume, qBound(0, volume, 100))) {
		updateAudioLevel();
		emit volumeChanged(m_volume);
	}
}

void PlayEngine::setPreamp(double preamp) {
	if (_ChangeZ(m_preamp, qBound(0.0, preamp, 10.0))) {
		updateAudioLevel();
		emit preampChanged(m_preamp);
	}
}

void PlayEngine::setMuted(bool muted) {
	if (_Change(m_muted, muted)) {
		d->enqueue(MpSetAudioMuted, "", (int)muted);
		setmp("mute", (int)m_muted);
		emit mutedChanged(m_muted);
	}
}

void PlayEngine::run() {
	QStringList args;
	args << "cmplayer-mpv";
	auto mpvOptions = qgetenv("CMPLAYER_MPV_OPTIONS").trimmed();
	if (!mpvOptions.isEmpty())
		args += QString::fromLocal8Bit(mpvOptions).split(' ', QString::SkipEmptyParts);
	args << "--no-config" << "--idle" << "--no-fs"
		<< ("--af=dummy:address=" % QString::number((quint64)(quintptr)(void*)(d->audio)))
		<< ("--vo=null:address=" % QString::number((quint64)(quintptr)(void*)(d->video)))
		<< "--softvol=yes" << "--softvol-max=1000.0" << "--fixed-vo" << "--no-autosub" << "--osd-level=0" << "--quiet" << "--identify"
		<< "--no-consolecontrols" << "--no-mouse-movements" << "--subcp=utf8" << "--ao=null,";
	QVector<QByteArray> args_byte(args.size());
	QVector<char*> args_raw(args.size());
	for (int i=0; i<args.size(); ++i) {
		args_byte[i] = args[i].toLocal8Bit();
		args_raw[i] = args_byte[i].data();
	}
	qDebug() << "Initialize mpv with" << args;
	auto mpctx = d->mpctx = create_player(args_raw.size(), args_raw.data());
	d->mpctx->priv = this;
	auto tmp_ao = d->mpctx->opts->audio_driver_list->name;
	d->init = true;
	d->quit = false;
	initialize_vaapi();
	initialize_vdpau();
	while (!d->quit) {
		m_imgMode = false;
		idle_player(mpctx);
		if (mpctx->stop_play == PT_QUIT)
			break;
		if (d->quit)
			break;
		Q_ASSERT(mpctx->playlist->current);
		clear();
		Mrl mrl = d->playlist.loadedMrl();
		d->playing = true;
		setState(EngineLoading);
		m_imgMode = mrl.isImage();
		int terminated = 0, duration = 0;
		int error = MPERROR_NONE;
		if (m_imgMode)
			error = playImage(mrl, terminated, duration);
		else
			error = playAudioVideo(mrl, terminated, duration);
		clean_up_playback(mpctx);
		if (error != MPERROR_NONE)
			setState(EngineError);
		d->playing = false;
		qDebug() << "terminate playback";
		if (mpctx->stop_play == PT_QUIT) {
			if (error == MPERROR_NONE) {
				setState(EngineStopped);
				postData(this, MrlStopped, d->playlist.loadedMrl(), terminated, duration);
			}
			break;
		}
		playlist_entry *entry = nullptr;
		if (error == MPERROR_NONE) {
			switch (mpctx->stop_play) {
			case KEEP_PLAYING:
			case AT_END_OF_FILE: {// finished
				setState(EngineFinished);
				postData(this, MrlFinished, mrl);
				playlist_clear(mpctx->playlist);
				if (d->playlist.hasNext()) {
					const auto prev = d->playlist.loadedMrl();
					d->playlist.setLoaded(d->playlist.next());
					const auto mrl = d->playlist.loadedMrl();
					if (prev != mrl)
						postData(this, MrlChanged, mrl);
					d->start = d->getStartTime(mrl);
					playlist_add(mpctx->playlist, playlist_entry_new(mrl.toString().toLocal8Bit()));
					entry = mpctx->playlist->first;
				} else
					postData(this, PlaylistFinished);
				break;
			} case PT_CURRENT_ENTRY: // stopped by loadfile
				entry = mpctx->playlist->current;
			default: // just stopped
				setState(EngineStopped);
				postData(this, MrlStopped, mrl, terminated, duration);
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
	mpctx->opts->hwdec_codecs = nullptr;
	mpctx->opts->hwdec_api = HWDEC_NONE;
	mpctx->opts->audio_driver_list->name = tmp_ao;
	destroy_player(mpctx);
	finalize_vaapi();
	finalize_vdpau();
	d->mpctx = nullptr;
	d->init = false;
	qDebug() << "terminate engine";
}

void PlayEngine::tellmp(const QString &cmd) {
	if (d->mpctx && d->mpctx->input) {
		mp_input_queue_cmd(d->mpctx->input, mp_input_parse_cmd(bstr0(cmd.toLocal8Bit().data()), ""));
	}
}

void PlayEngine::setVideoFilters(const QString &vfs) {
	if (_Change(d->vfs, vfs.toLocal8Bit())) {
		if (d->playing)
			tellmp("vf set", d->vfs);
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
		postData(this, MrlChanged, d->playlist.loadedMrl());
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
	case EngineError:
		play(d->getStartTime(d->playlist.loadedMrl()));
		break;
	case EngineLoading:
		// do nothing. just wait
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

void PlayEngine::setVolumeNormalizerActivated(bool on) {
	if (d->audio->setNormalizerActivated(on))
		emit volumeNormalizerActivatedChanged(on);
}

void PlayEngine::setTempoScalerActivated(bool on) {
	if (d->audio->setTempoScalerActivated(on)) {
		if (d->playing)
			d->enqueue(MpResetAudioChain);
		emit tempoScaledChanged(on);
	}
}

bool PlayEngine::isVolumeNormalized() const {
	return d->audio->isNormalizerActivated();
}

bool PlayEngine::isTempoScaled() const {
	return d->audio->isTempoScalerActivated();
}

void PlayEngine::stop() {
	tellmp("stop");
}

void PlayEngine::setVolumeNormalizerOption(double length, double target, double silence, double min, double max) {
	d->audio->setNormalizerOption(length, target, silence, min, max);
}

void PlayEngine::setDeintOptions(const DeintOption &swdec, const DeintOption &hwdec) {
	if (d->deint_swdec == swdec && d->deint_hwdec == hwdec)
		return;
	QMutexLocker locker(&d->mutex);
	if (d->deint_swdec == swdec && d->deint_hwdec == hwdec)
		return;
	d->deint_swdec = swdec;
	d->deint_hwdec = hwdec;
	d->enqueue(MpResetDeint);
}

void PlayEngine::setDeintEnabled(bool on) {
	if (d->deint == on)
		return;
	QMutexLocker locker(&d->mutex);
	if (d->deint == on)
		return;
	d->deint = on;
	d->enqueue(MpSetDeintEnabled);
}

bool PlayEngine::isDeintEanbled() const {
	return d->deint;
}
