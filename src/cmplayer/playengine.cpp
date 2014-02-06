#include "playengine.hpp"
#include "playengine_p.hpp"
#include "videorendereritem.hpp"
#include "globalqmlobject.hpp"
#include "submisc.hpp"
#include "translator.hpp"

extern "C" {
#include <input/keycodes.h>
#include <audio/decode/dec_audio.h>
#include <demux/demux.h>
}

struct PlaybackData {
	int begin = 0, duration = 0, terminated = 0;
	PlayEngine::State state = PlayEngine::Loading;
	PlayEngine::State newState = PlayEngine::Loading;
	ChapterList chapters;
	int cache = -1, chapter = -1;
	quint64 loops = 0;
	Mrl mrl;
	QVector<int> streams = {-1, 1, -1}; // vid aid sid
	QImage image;
};

struct PlayEngine::Data {
	Data(PlayEngine *engine): p(engine) {}
	PlayEngine *p = nullptr;

	ImagePlayback image;

	MediaInfoObject mediaInfo;
	AvInfoObject videoInfo, audioInfo;
	HardwareAcceleration hwAcc = HardwareAcceleration::Unavailable;

	bool hasImage = false;
	bool subStreamsVisible = true;

	Thread thread{p};
	AudioController *audio = nullptr;
	QByteArray fileName;
	QTimer imageTicker, avTicker;
	bool quit = false, looping = false, init = false;
	PlaybackData playback;
	int start = 0;
	bool muted = false;
	int volume = 100;
	double amp = 1.0;
	double speed = 1.0;
	qreal cache = -1.0;
	MPContext *mpctx = nullptr;
	VideoOutput *video = nullptr;
	GetMrlInt getStartTimeFunc, getCacheFunc;
	PlaylistModel playlist;
	QByteArray hwAccCodecs;
	QMutex mutex;
	QMap<QString, QString> subtitleNames;
	QList<SubtitleTempFile> subtitleFiles;
	ChannelLayout layout = ChannelLayout::Default;
	int duration = 0, audioSync = 0, begin = 0;
	int chapter = -2;
	QVector<int> streams = {-1, -1, -1};
	StreamList subStreams, audioStreams, videoStreams;
	AudioTrackInfoObject *audioTrackInfo = nullptr;
	VideoRendererItem *renderer = nullptr;
	DvdInfo dvd;
	ChapterList chapters;
	ChapterInfoObject *chapterInfo = nullptr;
	QByteArray vfs;

	QList<QMetaObject::Connection> rendererConnections;

	VideoFormat videoFormat;
	DeintOption deint_swdec, deint_hwdec;
	DeintMode deint = DeintMode::Auto;
	QByteArray ao = "";
	AudioDriver audioDriver = AudioDriver::Auto;

	int position = 0;

	SubtitleTrackInfoObject subtitleTrackInfo;

	void checkTimeRange() {
		if (_Change(playback.begin, int(get_start_time(mpctx)*1000))
				| _Change(playback.duration, int(get_time_length(mpctx)*1000))) {
			_PostEvent(p, TimeRangeChange, playback.begin, playback.duration);
			auto d = this;
			if (playback.duration) {
				auto &chapters = d->playback.chapters;
				chapters.resize(get_chapter_count(d->mpctx));
				for (int i=0; i<chapters.size(); ++i) {
					chapters[i].m_time = chapter_start_time(d->mpctx, i)*1000;
					const QString time = _MSecToString(chapters[i].m_time, _L("hh:mm:ss.zzz"));
					if (char *str = chapter_name(d->mpctx, i)) {
						chapters[i].m_name = QString::fromLocal8Bit(str);
						if (chapters[i].m_name != time)
							chapters[i].m_name += '(' % time % ')';
						talloc_free(str);
					} else
						chapters[i].m_name = time;
					chapters[i].m_id = i;
				}
				_PostEvent(p, UpdateChapterList, playback.chapters);
			}
		}
	}
	void checkCurrentStreams() {
		for (int i=0; i<STREAM_TYPE_COUNT; ++i) {
			if (_Change(playback.streams[i], mpctx->current_track[0][i] ? mpctx->current_track[0][i]->user_tid : -1))
				_PostEvent(p, UpdateCurrentStream, i, playback.streams[i]);
		}
	}

	static int mpCommandFilter(MPContext *mpctx, mp_cmd *cmd) {
		auto e = static_cast<PlayEngine*>(mpctx->priv); auto d = e->d;
		if (cmd->id < 0) {
			QMutexLocker locker(&d->mutex);
			switch (cmd->id) {
			case MpSetProperty:
				mp_property_do(cmd->name, M_PROPERTY_SET, &cmd->args[0].v, mpctx);
				break;
			case MpSetAudioLevel:
				d->audio->setAmpLevel(cmd->args[0].v.d);
				break;
			case MpSetAudioMuted:
				d->audio->setMuted(cmd->args[0].v.i);
				break;
			case MpSetTempoScaler:
				d->audio->setTempoScalerActivated(cmd->args[0].v.i);
				reinit_audio_chain(mpctx);
				break;
			case MpResetAudioChain:
				reinit_audio_chain(mpctx);
				break;
			case MpResetDeint:
				d->video->setDeintOptions(d->deint_swdec, d->deint_hwdec);
				break;
			case MpSetDeintEnabled:
				d->video->setDeintEnabled(d->deint != DeintMode::None);
				break;
			default:
				break;
			}
			cmd->id = MP_CMD_IGNORE;
			return true;
		}
		return false;
	}
	static int mpEventFilter(MPContext *mpctx) {
		enum MpEvent : uint {
			None = 1u << MP_EVENT_NONE,
			Tick = 1u << MP_EVENT_TICK,
			PropertyChanged = 1u << MP_EVENT_PROPERTY,
			TracksChanged = 1u << MP_EVENT_TRACKS_CHANGED,
			StartFile  = 1u << MP_EVENT_START_FILE,
			EndFile = 1u << MP_EVENT_END_FILE
		};
		auto e = static_cast<PlayEngine*>(mpctx->priv);
		int &events = *reinterpret_cast<int*>(mpctx->command_ctx);
		if (events & TracksChanged) {
			std::array<StreamList, STREAM_TYPE_COUNT> streams;
			QString name[STREAM_TYPE_COUNT];
			name[STREAM_AUDIO] = tr("Audio %1");
			name[STREAM_VIDEO] = tr("Video %1");
			name[STREAM_SUB] = tr("Subtitle %1");
			for (int i=0; i<mpctx->num_tracks; ++i) {
				const auto track = mpctx->tracks[i];
				auto &list = streams[track->type];
				if (!list.contains(track->user_tid)) {
					auto &stream = list[track->user_tid];
					stream.m_title = QString::fromLocal8Bit(track->title);
					stream.m_lang = QString::fromLocal8Bit(track->lang);
					if (stream.m_lang.size() == 2 || stream.m_lang.size() == 3) {
						for (int i=0; i<stream.m_lang.size(); ++i) {
							if (!_IsAlphabet(stream.m_lang[i].unicode()))
								goto skip_lang_conv;
						}
						stream.m_lang = Translator::displayLanguage(stream.m_lang);
					}
skip_lang_conv:
					stream.m_id = track->user_tid;
					stream.m_name = name[track->type].arg(track->user_tid);
				}
			}
			_PostEvent(e, UpdateTrack, streams);
		}
		events = 0;
		return true;
	}
	int getStartTime(const Mrl &mrl) { return getStartTimeFunc ? getStartTimeFunc(mrl) : 0; }
	int getCache(const Mrl &mrl) { return getCacheFunc ? getCacheFunc(mrl) : 0; }
	QByteArray &setFileName(const Mrl &mrl) {
		fileName = "\"";
		fileName += mrl.location().toLocal8Bit();
		fileName += "\"";
		return fileName;
	}
	template<typename T = int>
	bool enqueue(int id, const char *name = "", const T &v = 0) {
		const bool ret = mpctx && mpctx->input;
		if (ret) {
			mp_cmd_t *cmd = (mp_cmd_t*)talloc_ptrtype(NULL, cmd);
			cmd->id = id;
			cmd->name = (char*)name;
			if (std::is_same<bool, T>::value)
				getCmdArg<int>(cmd) = v;
			else
				getCmdArg<T>(cmd) = v;
			mp_input_queue_cmd(mpctx->input, cmd);
		}
		return ret;
	}
	void updateAudioLevel() { enqueue(MpSetAudioLevel, "", (double)(volume)*0.01*amp); }
	template<typename T> void setmp(const char *name, T value) { enqueue(MpSetProperty, name, value); }
	void tellmp(const QByteArray &cmd) {
		if (mpctx && mpctx->input)
			mp_input_queue_cmd(mpctx->input, mp_input_parse_cmd(mpctx->input, bstr0(cmd.data()), ""));
	}
	void tellmp(const QByteArray &cmd, std::initializer_list<QByteArray> list) {
		int size = cmd.size();
		for (auto &one : list) size += one.size();
		QByteArray str; str.reserve(size + list.size()*2 + 2); str = cmd;
		for (auto &one : list) { str += ' '; str += one; }
		tellmp(str);
	}
	template<typename T>
	void tellmp(const QByteArray &cmd, const T &arg) {
		tellmp(cmd, {qbytearray_from<T>(arg)});
	}
	template<typename T, typename S>
	void tellmp(const QByteArray &cmd, const T &a1, const S &a2) {
		tellmp(cmd, {qbytearray_from(a1), qbytearray_from(a2)});
	}
	template<typename T, typename S, typename R>
	void tellmp(const QByteArray &cmd, const T &a1, const S &a2, const R &a3) {
		tellmp(cmd, {qbytearray_from(a1), qbytearray_from(a2), qbytearray_from(a3)});
	}
//	template<template <typename> class T> void tellmp(const QByteArray &cmd, const T<QString> &args) {
//		QString c = cmd; for (auto arg : args) {c += _L(' ') % arg;} tellmp(c);
//	}

	void clear() {
		dvd.clear();
		audioStreams.clear();
		videoStreams.clear();
		subStreams.clear();
		subtitleFiles.clear();
		for (auto &f : subtitleFiles)
			delete f.file;
		subtitleFiles.clear();
		emit p->audioStreamsChanged(audioStreams);
		emit p->videoStreamsChanged(videoStreams);
		emit p->subtitleStreamsChanged(subStreams);
	}

	void play(int time) {
		start = time;
		tellmp("loadfile", setFileName(playlist.loadedMrl()), 0);
	}

	bool load(int row, int start) {
		if (!playlist.isValidRow(row))
			return false;
		if (playlist.loaded() == row) {
			if (start >= 0 && !looping)
				play(start);
		} else {
			playlist.setLoaded(row);
			_PostEvent(p, MrlChanged, playlist.loadedMrl());
			if (start < 0)
				p->stop();
			else
				play(start);
		}
		return true;
	}

	void updateMediaName() {
		QString name, category;
		auto mrl = p->mrl();
		if (mrl.isLocalFile())
			category = tr("File");
		else if (mrl.isDvd()) {
			category = _L("DVD");
			name = dvd.volume;
		} else
			category = _L("URL");
		if (name.isEmpty())
			name = mrl.displayName();
		mediaInfo.setName(category % _L(": ") % name);
	}
	HardwareAcceleration getHwAcc() const {
		auto dv = mpctx->d_video;
		if (dv && dv->header && dv->header->codec) {
			auto codec = dv->header->codec;
			if (HwAcc::supports(HwAcc::codecId(codec))) {
				if (video->hwAcc())
					return HardwareAcceleration::Activated;
				if (!hwAccCodecs.contains(codec))
					return HardwareAcceleration::Deactivated;
			}
		}
		return HardwareAcceleration::Unavailable;
	}
};

PlayEngine::PlayEngine()
: d(new Data(this)) {
	d->audio = new AudioController(this);
	d->video = new VideoOutput(this);
	d->chapterInfo = new ChapterInfoObject(this, this);
	d->audioTrackInfo = new AudioTrackInfoObject(this, this);
	d->imageTicker.setInterval(20);
	d->avTicker.setInterval(100);
	d->updateMediaName();
	static auto stageNotifier = [] (MPContext *mpctx, int stage) {
		static_cast<PlayEngine*>(mpctx->priv)->onMpvStageChanged(stage);
	};
	mp_register_player_stage_notifier(stageNotifier);
	mp_register_player_command_filter(Data::mpCommandFilter);
	mp_register_player_event_filter(Data::mpEventFilter);

	connect(&d->imageTicker, &QTimer::timeout, [this] () {
		bool begin = false, duration = false, pos = false;
		if (d->hasImage) {
			pos = _Change(d->position, d->image.pos());
			begin = _Change(d->duration, d->image.duration());
			duration = _Change(d->begin, 0);
		}
		if (pos)
			emit tick(d->position);
		if (begin)
			emit beginChanged(d->begin);
		if (duration)
			emit durationChanged(d->duration);
		if (begin || duration)
			emit endChanged(end());
		if (pos || begin || duration)
			emit relativePositionChanged();
	});
	connect(&d->avTicker, &QTimer::timeout, [this] () {
		d->position = get_current_time(d->mpctx)*1000.0 + 0.5;
		emit tick(d->position);
		emit relativePositionChanged();
	});
	connect(d->video, &VideoOutput::formatChanged, [this] (const VideoFormat &format) {
		_PostEvent(this, VideoFormatChanged, format);
		_PostEvent(this, HwAccChanged, d->getHwAcc());
	});
	connect(d->video, &VideoOutput::hwAccChanged, [this] () {
		_PostEvent(this, HwAccChanged, d->getHwAcc());
	});
	connect(&d->playlist, &PlaylistModel::playRequested, [this] (int row) {
		d->load(row, d->getStartTime(d->playlist[row]));
	});
}

PlayEngine::~PlayEngine() {
	delete d->chapterInfo;
	delete d->audioTrackInfo;
	delete d->audio;
	delete d->video;
//	finalizeGL();
	delete d;
}

SubtitleTrackInfoObject *PlayEngine::subtitleTrackInfo() const {
	return &d->subtitleTrackInfo;
}

void PlayEngine::setSubtitleTracks(const QStringList &tracks) {
	d->subtitleTrackInfo.set(tracks);
	emit subtitleTrackInfoChanged();
}

void PlayEngine::setCurrentSubtitleIndex(int idx) {
	d->subtitleTrackInfo.setCurrentIndex(idx);
}

ChapterInfoObject *PlayEngine::chapterInfo() const {
	return d->chapterInfo;
}

AudioTrackInfoObject *PlayEngine::audioTrackInfo() const {
	return d->audioTrackInfo;
}

qreal PlayEngine::cache() const {
	return d->cache;
}

int PlayEngine::begin() const { return d->begin; }
int PlayEngine::end() const { return d->begin + d->duration; }

void PlayEngine::setImageDuration(int duration) {
	d->image.setDuration(duration);
}

int PlayEngine::duration() const {
	return d->duration;
}

const DvdInfo &PlayEngine::dvd() const {return d->dvd;}
int PlayEngine::currentDvdTitle() const {return d->dvd.currentTitle;}
const ChapterList &PlayEngine::chapters() const {return d->chapters;}

const StreamList &PlayEngine::subtitleStreams() const {return d->subStreams;}

VideoRendererItem *PlayEngine::videoRenderer() const {return d->renderer;}

const StreamList &PlayEngine::videoStreams() const {return d->videoStreams;}

int PlayEngine::audioSync() const {return d->audioSync;}
const StreamList &PlayEngine::audioStreams() const {return d->audioStreams;}

PlayEngine::HardwareAcceleration PlayEngine::hwAcc() const {
	return d->hwAcc;
}

QString PlayEngine::hwAccText() const {
	switch (d->hwAcc) {
	case HardwareAcceleration::Activated:
		return tr("Activated");
		break;
	case HardwareAcceleration::Deactivated:
		return tr("Deactivated");
		break;
	default:
		return tr("Unavailable");
	}
}

void PlayEngine::run() {
	d->thread.start();
}

QThread *PlayEngine::thread() const {
	return &d->thread;
}

void PlayEngine::waitUntilTerminated() {
	d->thread.wait();
}

void PlayEngine::waitUntilInitilaized() {
	while (!d->init)
		QThread::msleep(1);
//	d->ticker.start();
	d->videoInfo.setVideo(this);
	d->audioInfo.setAudio(this);
}

double PlayEngine::speed() const {
	return d->speed;
}

void PlayEngine::setSpeed(double speed) {
	if (_ChangeZ(d->speed, speed)) {
		d->setmp("speed", speed);
		emit speedChanged(d->speed);
	}
}

void PlayEngine::seek(int pos) {
	d->chapter = -1;
	if (d->hasImage)
		d->image.seek(pos, false);
	else
		d->tellmp("seek", (double)pos/1000.0, 2);
}

void PlayEngine::relativeSeek(int pos) {
	if (d->hasImage)
		d->image.seek(pos, true);
	else
		d->tellmp("seek", (double)pos/1000.0, 0);
	emit sought();
}

void PlayEngine::setClippingMethod(ClippingMethod method) {
	d->audio->setClippingMethod(method);
}

void PlayEngine::setChannelLayoutMap(const ChannelLayoutMap &map) {
	d->audio->setChannelLayoutMap(map);
}

void PlayEngine::setChannelLayout(ChannelLayout layout) {
	(_Change(d->layout, layout));
//		d->enqueue(MpSetAudioLayout, "", (int)d->layout);
}

typedef QPair<AudioDriver, const char*> AudioDriverName;
const std::array<AudioDriverName, AudioDriverInfo::size()-1> audioDriverNames = {{
	{AudioDriver::ALSA, "alsa"},
	{AudioDriver::OSS, "oss"},
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

QQuickItem *PlayEngine::screen() const {
	return d->renderer;
}

AudioDriver PlayEngine::preferredAudioDriver() const {
	return d->audioDriver;
}

AudioDriver PlayEngine::audioDriver() const {
	if (!d->mpctx->ao)
		return preferredAudioDriver();
	auto name = d->mpctx->ao->driver->name;
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

double PlayEngine::volumeNormalizer() const {
	auto gain = d->audio->gain(); return gain < 0 ? 1.0 : gain;
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

bool PlayEngine::isSubtitleStreamsVisible() const {return d->subStreamsVisible;}

void PlayEngine::setSubtitleStreamsVisible(bool visible) {
	d->subStreamsVisible = visible;
	const auto id = currentSubtitleStream();
	d->setmp("sub-visibility", (d->subStreamsVisible && id >= 0));
}

void PlayEngine::setCurrentSubtitleStream(int id) {
	d->setmp("sub-visibility", (d->subStreamsVisible && id >= 0));
	d->setmp("sub", id);
}

int PlayEngine::currentSubtitleStream() const {
	return d->streams[STREAM_SUB];
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
		SubtitleTempFile item;
		item.info.path = info.absoluteFilePath();
		item.info.encoding = enc;
		item.file = out;
		d->subtitleFiles.append(item);
		d->subtitleNames[out->fileName()] = info.fileName();
		d->tellmp("sub_add", out->fileName());
	}
}

void PlayEngine::removeSubtitleStream(int id) {
	auto it = d->subStreams.find(id);
	if (it != d->subStreams.end()) {
		auto fileName = it->fileName();
		if (!fileName.isEmpty()) {
			d->subtitleNames.remove(fileName);
			for (int i=0; i<d->subtitleFiles.size(); ++i) {
				if (d->subtitleFiles[i].file->fileName() == fileName) {
					delete d->subtitleFiles.takeAt(i).file;
					break;
				}
			}
		}
		d->tellmp("sub_remove", id);
	}
}

double PlayEngine::avgfps() const {
	return d->renderer->avgfps();
}

double PlayEngine::avgsync() const {
	double sync = 0.0;
	if (d->mpctx) {
		if (d->mpctx->d_audio && d->mpctx->d_audio->header && d->mpctx->d_video && d->mpctx->d_video->header)
			sync = (d->mpctx->last_av_difference)*1000.0;
		if (d->renderer)
			sync -= d->renderer->delay();
	}
	return sync;
}

template<typename T>
static bool _CheckSwap(T &the, T &one) { if (the != one) { the.swap(one); return true; } return false; }

void PlayEngine::customEvent(QEvent *event) {
	switch ((int)event->type()) {
	case UpdateChapterList: {
		auto chapters = _GetData<ChapterList>(event);
		if (_CheckSwap(d->chapters, chapters)) {
			d->chapterInfo->setCount(d->chapters.size());
			emit chaptersChanged(d->chapters);
		}
		break;
	} case UpdateDVDInfo:
		d->dvd = _GetData<DvdInfo>(event);
		emit dvdInfoChanged();
		break;
	case UpdateCache:
		d->cache = _GetData<int>(event)*1e-2;
		emit cacheChanged();
		break;
	case UpdateCurrentChapter:
		d->chapter = _GetData<int>(event);
		emit currentChapterChanged(d->chapter);
		break;
	case UpdateCurrentStream: {
		int type, id;
		_GetAllData(event, type, id);
		d->streams[type] = id;
		switch (type) {
		case STREAM_AUDIO:
			emit currentAudioStreamChanged(id);
			break;
		case STREAM_VIDEO:
			emit currentVideoStreamChanged(id);
			break;
		case STREAM_SUB:
			emit currentSubtitleStreamChanged(id);
			break;
		default:
			break;
		}
		break;
	} case UpdateTrack: {
		auto streams = _GetData<std::array<StreamList, STREAM_TYPE_COUNT>>(event);
		if (_CheckSwap(d->videoStreams, streams[STREAM_VIDEO])) {
			emit videoStreamsChanged(d->videoStreams);
			emit hasVideoChanged();
		}
		if (_CheckSwap(d->audioStreams, streams[STREAM_AUDIO])) {
			d->audioTrackInfo->setCount(d->audioStreams.size());
			emit audioStreamsChanged(d->audioStreams);
		}
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
		if (_CheckSwap(d->subStreams, streams[STREAM_SUB]))
			emit subtitleStreamsChanged(d->subStreams);
		break;
	} case StreamOpen:
		d->updateMediaName();
		d->audioInfo.setAudio(this);
		d->start = 0;
		d->position = 0;
		d->cache = -1;
		if (d->renderer)
			d->renderer->reset();
		emit tick(d->position);
		emit seekableChanged(isSeekable());
		emit started(d->playlist.loadedMrl());
		emit mediaChanged();
		emit audioChanged();
		emit cacheChanged();
		break;
	case StateChange: {
		const auto state = _GetData<PlayEngine::State>(event);
		const bool wasRunning = isRunning();
		if (_Change(m_state, state)) {
			emit stateChanged(m_state);
			if (m_state & (Playing | Paused)) {
				if (d->hasImage)
					d->imageTicker.start();
				else if (d->mpctx && d->mpctx->demuxer)
					d->avTicker.start();
			} else {
				if (d->hasImage)
					d->imageTicker.stop();
				else
					d->avTicker.stop();
			}
			if (wasRunning != isRunning())
				emit runningChanged();
		}
		break;
	} case MrlStopped: {
		Mrl mrl; int terminated = 0, duration = 0;
		_GetAllData(event, mrl, terminated, duration);
		emit stopped(mrl, terminated, duration);
		break;
	} case MrlFinished: {
		const auto mrl = _GetData<Mrl>(event);
		emit finished(mrl);
		break;
	} case PlaylistFinished:
		emit d->playlist.finished();
		break;
	case TimeRangeChange:
		_GetAllData(event, d->begin, d->duration);
		emit durationChanged(d->duration);
		emit beginChanged(d->begin);
		emit endChanged(end());
		break;
	case MrlChanged: {
		const auto mrl = _GetData<Mrl>(event);
		d->updateMediaName();
		emit mrlChanged(mrl);
		emit mediaChanged();
	} case VideoFormatChanged: {
		const auto format = _GetData<VideoFormat>(event);
		if (_Change(d->videoFormat, format)) {
			d->videoInfo.setVideo(this);
			emit videoFormatChanged(d->videoFormat);
			emit videoChanged();
		}
	} case HwAccChanged:
		if (_Change(d->hwAcc, _GetData<HardwareAcceleration>(event)))
			emit hwAccChanged();
	default:
		break;
	}
}

MediaInfoObject *PlayEngine::mediaInfo() const {
	return &d->mediaInfo;
}

AvInfoObject *PlayEngine::audioInfo() const {
	return &d->audioInfo;
}

AvInfoObject *PlayEngine::videoInfo() const {
	return &d->videoInfo;
}

void PlayEngine::setState(PlayEngine::State state) {
	_PostEvent(this, StateChange, state);
}

void PlayEngine::setCurrentChapter(int id) {
	d->setmp("chapter", id);
}

void PlayEngine::setCurrentDvdTitle(int id) {
	auto mrl = d->playlist.loadedMrl();
	if (mrl.isDvd()) {
		const QString path = "dvdnav://" % QString::number(id) % mrl.location().mid(6);
		d->fileName = path.toLocal8Bit();
		d->tellmp("loadfile", path, 0);
	}
}

void PlayEngine::sendDVDCommand(DVDCmd cmd) {
	if (!d->playback.mrl.isDvd())
		return;
	switch (cmd) {
	case DVDMenu:
		mp_nav_user_input(d->mpctx, const_cast<char*>("menu"));
		break;
	}
}

MPContext *PlayEngine::context() const {
	return d->mpctx;
}

bool PlayEngine::isInitialized() const {
	return d->init;
}

void PlayEngine::onMpvStageChanged(int stage) {
	switch (stage) {
	case MP_STAGE_START_IDLING: {
		while (d->mpctx->playlist->current && d->mpctx->playlist->current->prev)
			playlist_remove(d->mpctx->playlist, d->mpctx->playlist->current->prev);
		break;
	} case MP_STAGE_PREPARE_TO_OPEN: {
		d->clear();
		d->playback = PlaybackData();
		d->video->output(QImage());
		d->looping = true;
		setState(PlayEngine::Loading);

		d->playback.mrl = d->playlist.loadedMrl();
		d->mpctx->opts->pause = d->hasImage = d->playback.mrl.isImage();
		if (d->hwAccCodecs.isEmpty() || d->hasImage) {
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
		auto &start = d->mpctx->opts->play_start;
		if (!d->playback.mrl.isDvd()) {
			start.pos = d->start*1e-3;
			start.type = REL_TIME_ABSOLUTE;
		} else {
			start.pos = 0.0;
			start.type = REL_TIME_NONE;
		}
		d->setmp("audio-delay", d->audioSync*0.001);
		d->video->setDeintOptions(d->deint_swdec, d->deint_hwdec);
		d->video->setDeintEnabled(d->deint != DeintMode::None);
		break;
	} case MP_STAGE_OPEN_STREAM:
		break;
	case MP_STAGE_OPEN_DEMUXER: {
		d->updateAudioLevel();
		DvdInfo dvd;
		if (d->mpctx->stream && d->mpctx->stream->info && !strcmp(d->mpctx->stream->info->name, "dvd")) {
			char buffer[256];
			if (dvd_volume_id(d->mpctx->stream, buffer, sizeof(buffer)))
				dvd.volume = QString::fromLocal8Bit(buffer);
			int titles = 0;
			stream_control(d->mpctx->stream, STREAM_CTRL_GET_NUM_TITLES, &titles);
			stream_control(d->mpctx->stream, STREAM_CTRL_GET_CURRENT_TITLE, &dvd.currentTitle);
			dvd.titles.resize(titles);
			for (int tid=0; tid<titles; ++tid) {
				auto &title = dvd.titles[tid];
				title.m_id = tid;
				title.m_name = tr("Title %1").arg(tid+1);
			}
		}
		_PostEvent(this, UpdateDVDInfo, dvd);
		uint title = 0, titles = 0;
		if (d->mpctx->demuxer && d->mpctx->demuxer->stream) {
			auto stream = d->mpctx->demuxer->stream;
			stream->control(stream, STREAM_CTRL_GET_CURRENT_TITLE, &title);
			stream->control(stream, STREAM_CTRL_GET_NUM_TITLES, &titles);
		}
		_PostEvent(this, StreamOpen);
		d->tellmp("vf set", d->vfs);
		d->playback.chapter = d->chapter;
		d->checkTimeRange();
		break;
	} case MP_STAGE_START_PLAYBACK:
		break;
	case MP_STAGE_IN_PLAYLOOP:
		if (!d->playback.duration)
			d->checkTimeRange();
		if (d->playback.loops++) {
			auto state = Playing;
			if (d->mpctx->paused_for_cache && !d->mpctx->opts->pause)
				state = Loading;
			else if (d->mpctx->paused)
				state = Paused;
			if (_Change(d->playback.state, state))
				setState(state);
			if (_Change(d->playback.cache, mp_get_cache_percent(d->mpctx)))
				_PostEvent(this, UpdateCache, d->playback.cache);
			if (_Change(d->playback.chapter, !d->chapters.isEmpty() ? get_current_chapter(d->mpctx) : -2))
				_PostEvent(this, UpdateCurrentChapter, d->playback.chapter);
			d->checkCurrentStreams();
		}
		break;
	case MP_STAGE_TERMINATE_PLAYBACK:
		d->playback.terminated = time();
		d->looping = false;

		if (d->mpctx->error_playing) {
			if (d->mpctx->stop_play == PT_QUIT) {
				setState(Stopped);
				_PostEvent(this, MrlStopped, d->playback.mrl, d->playback.terminated, d->playback.duration);
			} else
				setState(Error);
		} else {
			switch (d->mpctx->stop_play) {
			case KEEP_PLAYING:
			case AT_END_OF_FILE: {// finished
				setState(Finished);
				_PostEvent(this, MrlFinished, d->playback.mrl);
				if (d->playlist.hasNext()) {
					d->playlist.setLoaded(d->playlist.next());
					const auto mrl = d->playlist.loadedMrl();
					if (d->playback.mrl != mrl)
						_PostEvent(this, MrlChanged, mrl);
					d->start = d->getStartTime(mrl);
					playlist_add(d->mpctx->playlist, playlist_entry_new(mrl.location().toLocal8Bit()));
				} else
					_PostEvent(this, PlaylistFinished);
				break;
			} default: // just stopped
				setState(Stopped);
				_PostEvent(this, MrlStopped, d->playback.mrl, d->playback.terminated, d->playback.duration);
				break;
			}
		}
		break;
	}
}


void PlayEngine::setVolume(int volume) {
	if (_Change(d->volume, qBound(0, volume, 100))) {
		d->updateAudioLevel();
		emit volumeChanged(d->volume);
	}
}

bool PlayEngine::isMuted() const {
	return d->muted;
}

int PlayEngine::volume() const {
	return d->volume;
}

double PlayEngine::amp() const {
	return d->amp;
}

void PlayEngine::setAmp(double amp) {
	if (_ChangeZ(d->amp, qBound(0.0, amp, 10.0))) {
		d->updateAudioLevel();
		emit preampChanged(d->amp);
	}
}

void PlayEngine::setMuted(bool muted) {
	if (_Change(d->muted, muted)) {
		d->enqueue(MpSetAudioMuted, "", (int)muted);
		d->setmp("mute", (int)d->muted);
		emit mutedChanged(d->muted);
	}
}

void PlayEngine::exec() {
	QStringList args;
	args << "cmplayer-mpv";
	auto mpvOptions = qgetenv("CMPLAYER_MPV_OPTIONS").trimmed();
	if (!mpvOptions.isEmpty())
		args += QString::fromLocal8Bit(mpvOptions).split(' ', QString::SkipEmptyParts);
	args << "--no-config" << "--idle" << "--no-fs" << "--mouse-movements" << "-v" << "--identify"
		<< ("--af=dummy:address=" % QString::number((quint64)(quintptr)(void*)(d->audio)))
		<< ("--vo=null:address=" % QString::number((quint64)(quintptr)(void*)(d->video)))
		<< "--softvol=yes" << "--softvol-max=1000.0" << "--fixed-vo" << "--no-autosub" << "--osd-level=0" << "--quiet"
		<< "--no-consolecontrols" << "--no-mouse-movements" << "--subcp=utf8" << "--ao=null,"
		<< "--ad-lavc-downmix=no";

	QVector<QByteArray> args_byte(args.size());
	QVector<char*> args_raw(args.size());
	for (int i=0; i<args.size(); ++i) {
		args_byte[i] = args[i].toLocal8Bit();
		args_raw[i] = args_byte[i].data();
	}
	qDebug() << "Initialize mpv with" << args;
	auto mpctx = d->mpctx = mpv_main(args_raw.size(), args_raw.data());
	d->mpctx->priv = this;
	auto tmp_ao = d->mpctx->opts->audio_driver_list->name;
	d->init = true;
	d->quit = false;
	initialize_vaapi();
	initialize_vdpau();
	mp_play_files(mpctx);
	d->hasImage = false;
	qDebug() << "terminate loop";
	mpctx->opts->hwdec_codecs = nullptr;
	mpctx->opts->hwdec_api = HWDEC_NONE;
	mpctx->opts->audio_driver_list->name = tmp_ao;
	exit_player(mpctx, EXIT_QUIT);
	finalize_vaapi();
	finalize_vdpau();
	d->mpctx = nullptr;
	d->init = false;
	qDebug() << "terminate engine";
}

void PlayEngine::setVideoFilters(const QString &vfs) {
	if (_Change(d->vfs, vfs.toLocal8Bit()) && d->looping)
		d->tellmp("vf set", d->vfs);
}

void PlayEngine::quit() {
	d->tellmp("quit 1");
}

void PlayEngine::reload() {
	d->play(time());
}

void PlayEngine::load(const Mrl &mrl, bool play) {
	load(mrl, play ? d->getStartTime(mrl) : -1);
}

void PlayEngine::load(const Mrl &mrl, int start) {
	auto row = d->playlist.rowOf(mrl);
	if (row < 0)
		row = d->playlist.append(mrl);
	d->load(row, start);
}

int PlayEngine::time() const {
	return d->position;
}

bool PlayEngine::isSeekable() const {
	return d->mpctx && d->mpctx->stream && d->mpctx->stream->seek && (!d->mpctx->demuxer || d->mpctx->demuxer->seekable);
}

bool PlayEngine::hasVideo() const {
	return !d->videoStreams.isEmpty();
}

bool PlayEngine::atEnd() const {
	return d->mpctx->stop_play == AT_END_OF_FILE;
}

int PlayEngine::currentChapter() const {
	if (d->looping)
		return d->chapter;
	return -2;
}

void PlayEngine::pause() {
	if (d->hasImage)
		setState(PlayEngine::Paused);
	else
		d->setmp("pause", 1);
}

void PlayEngine::unpause() {
	if (d->hasImage)
		setState(PlayEngine::Playing);
	else
		d->setmp("pause", 0);
}

void PlayEngine::play() {
	switch (m_state) {
	case PlayEngine::Stopped:
	case PlayEngine::Finished:
	case PlayEngine::Error:
		d->play(d->getStartTime(d->playlist.loadedMrl()));
		break;
	case PlayEngine::Loading:
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

int PlayEngine::currentAudioStream() const {
	return d->streams[STREAM_AUDIO];
}

void PlayEngine::setCurrentVideoStream(int id) {
	d->setmp("video", id);
}

int PlayEngine::currentVideoStream() const {
	return hasVideo() ? d->streams[STREAM_VIDEO] : -1;
}

void PlayEngine::setCurrentAudioStream(int id) {
	d->setmp("audio", id);
}

void PlayEngine::setAudioSync(int sync) {
	if (_Change(d->audioSync, sync))
		d->setmp("audio-delay", (float)(sync*0.001));
}

const PlaylistModel &PlayEngine::playlist() const {
	return d->playlist;
}

PlaylistModel &PlayEngine::playlist() {
	return d->playlist;
}

double PlayEngine::fps() const {
	return hasVideo() && d->mpctx->d_video ? d->mpctx->d_video->fps : 25;
}

void PlayEngine::setVideoRenderer(VideoRendererItem *renderer) {
	if (d->renderer != renderer) {
		for (auto &conn : d->rendererConnections)
			disconnect(conn);
		d->rendererConnections.clear();
		d->renderer = renderer;
		d->video->setRenderer(d->renderer);
		if (d->renderer)
			d->rendererConnections << connect(d->renderer
				, &VideoRendererItem::droppedFramesChanged, this, &PlayEngine::droppedFramesChanged);
	}
}

int PlayEngine::droppedFrames() const {
	return d->renderer ? d->renderer->droppedFrames() : 0;
}

double PlayEngine::bps(double fps) const {
	return d->videoFormat.bps(fps);
}

VideoFormat PlayEngine::videoFormat() const {
	return d->videoFormat;
}

void PlayEngine::registerObjects() {
	static auto utilProvider = [](QQmlEngine *, QJSEngine *) -> QObject* {return new UtilObject;};
	static auto settingsProvider = [](QQmlEngine *, QJSEngine *) -> QObject* {return new SettingsObject;};

	qRegisterMetaType<PlayEngine::State>("State");
	qRegisterMetaType<Mrl>("Mrl");
	qRegisterMetaType<VideoFormat>("VideoFormat");
	qRegisterMetaType<QVector<int>>("QVector<int>");
	qRegisterMetaType<StreamList>("StreamList");
	qmlRegisterType<QAction>();
	qmlRegisterType<ChapterInfoObject>();
	qmlRegisterType<AudioTrackInfoObject>();
	qmlRegisterType<SubtitleTrackInfoObject>();
	qmlRegisterType<AvInfoObject>();
	qmlRegisterType<AvIoFormat>();
	qmlRegisterType<MediaInfoObject>();
	qmlRegisterType<PlayEngine>("CMPlayer", 1, 0, "Engine");
	qmlRegisterSingletonType<UtilObject>("CMPlayer", 1, 0, "Util", utilProvider);
	qmlRegisterSingletonType<SettingsObject>("CMPlayer", 1, 0, "Settings", settingsProvider);
}

void PlayEngine::setVolumeNormalizerActivated(bool on) {
	if (d->audio->isNormalizerActivated() != on) {
		d->audio->setNormalizerActivated(on);
		emit volumeNormalizerActivatedChanged(on);
	}
}

void PlayEngine::setTempoScalerActivated(bool on) {
	if (d->audio->isTempoScalerActivated() != on) {
		d->enqueue(MpSetTempoScaler, "", (int)on);
		emit tempoScaledChanged(on);
	}
}

bool PlayEngine::isVolumeNormalizerActivated() const {
	return d->audio->isNormalizerActivated();
}

bool PlayEngine::isTempoScaled() const {
	return d->audio->isTempoScalerActivated();
}

void PlayEngine::stop() {
	d->tellmp("stop");
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

void PlayEngine::setDeintMode(DeintMode mode) {
	if (d->deint == mode)
		return;
	QMutexLocker locker(&d->mutex);
	if (d->deint == mode)
		return;
	d->deint = mode;
	d->enqueue(MpSetDeintEnabled);
}

DeintMode PlayEngine::deintMode() const {
	return d->deint;
}

QString PlayEngine::stateText(State state) {
	switch (state) {
	case Playing:
		return tr("Playing");
	case Stopped:
		return tr("Stopped");
	case Finished:
		return tr("Finished");
	case Loading:
		return tr("Loading");
	case Error:
		return tr("Error");
	default:
		return tr("Paused");
	}
}

QString PlayEngine::stateText() const { return stateText(m_state); }

void PlayEngine::sendMouseClick(const QPointF &pos) {
	sendMouseMove(pos);
	if (d->mpctx && d->init && d->mpctx->input) {
		mp_input_put_key(d->mpctx->input, MP_MOUSE_BTN0 | MP_KEY_STATE_DOWN);
		mp_input_put_key(d->mpctx->input, MP_MOUSE_BTN0 | MP_KEY_STATE_UP);
	}
}

void PlayEngine::sendMouseMove(const QPointF &pos) {
	if (d->init && d->video)
		mp_input_set_mouse_pos(d->mpctx->input, pos.x(), pos.y());
}

QList<SubtitleFileInfo> PlayEngine::subtitleFiles() const {
	QList<SubtitleFileInfo> list;
	list.reserve(d->subtitleFiles.size());
	for (auto &f : d->subtitleFiles)
		list.append(f.info);
	return list;
}
