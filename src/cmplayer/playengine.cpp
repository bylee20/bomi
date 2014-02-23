#include "playengine.hpp"
#include "playengine_p.hpp"
#include "videorendereritem.hpp"
#include "globalqmlobject.hpp"
#include "submisc.hpp"
#include "translator.hpp"
#include "log.hpp"

DECLARE_LOG_CONTEXT(Engine)
#include <libmpv/client.h>
extern "C" {
struct mpv_handle {
	// -- immmutable
	char *name;
	struct mp_log *log;
	struct MPContext *mpctx;
};
#include <input/keycodes.h>
#include <audio/decode/dec_audio.h>
#include <demux/demux.h>
}

static QByteArray doubleQuoted(const QString &fileName) {
	const auto file = fileName.toLocal8Bit();
	QByteArray arg; arg.reserve(file.size() + 2);
	arg += '"'; arg += file; arg += '"';
	return arg;
}

struct PlayEngine::Data {
	Data(PlayEngine *engine): p(engine) {}
	PlayEngine *p = nullptr;

	ImagePlayback image;

	MediaInfoObject mediaInfo;
	AvInfoObject videoInfo, audioInfo;
	HardwareAcceleration hwAcc = HardwareAcceleration::Unavailable;

	bool hasImage = false;
	bool subStreamsVisible = true, userPaused = false, startPaused = false;

	mpv_error errorStatus = MPV_ERROR_SUCCESS;
	const char *error() const { return mpv_error_string(errorStatus); }
	bool isSuccess() const { return errorStatus == MPV_ERROR_SUCCESS; }
	bool isSuccess(int error) { errorStatus = (mpv_error)error; return isSuccess(); }
	template<class... Args>
	bool check(int err, const char *msg, const Args &... args) {
		if (isSuccess(err))
			return true;
		Log::write(getLogContext(), Log::Error, _ByteArrayLiteral("Error %%: %%"), error(), Log::parse(msg, args...));
		return false;
	}
	template<class... Args>
	void fatal(int err, const char *msg, const Args &... args) {
		if (!isSuccess(err))
			Log::write(getLogContext(), Log::Fatal, _ByteArrayLiteral("Error %%: %% Exit..."), error(), Log::parse(msg, args...));
	}
	void setOption(const char *name, const char *data) {
		fatal(mpv_set_option_string(handle, name, data), "Couldn't set option %%=%%.", name, data);
	}

	Thread thread{p};
	AudioController *audio = nullptr;
	QTimer imageTicker;
	bool quit = false, init = false;
	bool muted = false, tick = false;
	int volume = 100;
	double amp = 1.0, speed = 1.0;
	int cacheForPlayback = 20, cacheForSeeking = 50;
	qreal cache = -1.0;
	MPContext *mpctx = nullptr;
	mpv_handle *handle = nullptr;
	VideoOutput *video = nullptr;
	QByteArray hwaccCodecs;
	QMutex mutex;
	QList<SubtitleFileInfo> subtitleFiles;
	ChannelLayout layout = ChannelLayout::Default;
	int duration = 0, audioSync = 0, begin = 0, position = 0, subDelay = 0, chapter = -2;
	QVector<int> streamIds = {-1, -1, -1};
	QVector<StreamList> streams = {StreamList(), StreamList(), StreamList()};
	AudioTrackInfoObject *audioTrackInfo = nullptr;
	VideoRendererItem *renderer = nullptr;
	DvdInfo dvd;
	ChapterList chapters, chapterFakeList;
	ChapterInfoObject *chapterInfo = nullptr;

	QList<QMetaObject::Connection> rendererConnections;

	VideoFormat videoFormat;
	DeintOption deint_swdec, deint_hwdec;
	DeintMode deint = DeintMode::Auto;
	QByteArray ao = "";
	AudioDriver audioDriver = AudioDriver::Auto;

	StartInfo startInfo, nextInfo;

	SubtitleTrackInfoObject subtitleTrackInfo;

	static int mpCommandFilter(MPContext *mpctx, mp_cmd *cmd) {
		auto e = static_cast<PlayEngine*>(mpctx->priv); auto d = e->d;
		if (cmd->id < 0) {
			QMutexLocker locker(&d->mutex);
			switch (cmd->id) {
			case MpSetProperty:
				mp_property_do(cmd->name, M_PROPERTY_SET, &cmd->args[0].v, mpctx);
				break;
			case MpSetTempoScaler:
				d->audio->setTempoScalerActivated(cmd->args[0].v.i);
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
	double mpVolume() const { return volume*amp/10.0; }
	template<typename T>
	void setmpv(const char *name, T value) {
		mpv_set_property_async(handle, 0, name, MPV_FORMAT_STRING, qbytearray_from<T>(value).data());
	}

	template<typename T> void setmp(const char *name, T value) { enqueue(MpSetProperty, name, value); }
	void tellmpv(const QByteArray &cmd) {
		check(mpv_command_string(handle, cmd.constData()), "Cannaot execute: %%", cmd);
	}
	void tellmpv(const QByteArray &cmd, std::initializer_list<QByteArray> list) {
		int size = cmd.size();
		for (auto &one : list) size += one.size();
		QByteArray str; str.reserve(size + list.size()*2 + 2); str = cmd;
		for (auto &one : list) { str += ' '; str += one; }
		tellmpv(str);
	}
	template<typename T>
	void tellmpv(const QByteArray &cmd, const T &arg) {
		tellmpv(cmd, {qbytearray_from<T>(arg)});
	}
	template<typename T, typename S>
	void tellmpv(const QByteArray &cmd, const T &a1, const S &a2) {
		tellmpv(cmd, {qbytearray_from<T>(a1), qbytearray_from<S>(a2)});
	}
	template<typename T, typename S, typename R>
	void tellmpv(const QByteArray &cmd, const T &a1, const S &a2, const R &a3) {
		tellmpv(cmd, {qbytearray_from<T>(a1), qbytearray_from<S>(a2), qbytearray_from<R>(a3)});
	}
//	template<template <typename> class T> void tellmp(const QByteArray &cmd, const T<QString> &args) {
//		QString c = cmd; for (auto arg : args) {c += _L(' ') % arg;} tellmp(c);
//	}

	void updateMrl() {
		hasImage = startInfo.mrl.isImage();
		updateMediaName();
		emit p->mrlChanged(startInfo.mrl);
		emit p->mediaChanged();
	}

	void clear() {
		dvd.clear();

	}
	void loadfile(const QByteArray &file, int resume, int cache) {
		if (file.isEmpty())
			return;
		QByteArray cmd = "loadfile \"" + file + "\" replace ";
		cmd += "ao=" + (ao.isEmpty() ? "\"\"" : ao);
		if (hwaccCodecs.isEmpty())
			cmd += ",hwdec=no";
		else {
#ifdef Q_OS_LINUX
			if (HwAcc::backend() == HwAcc::VdpauX11)
				cmd += ",hwdec=vdpau";
			else
				cmd += ",hwdec=vaapi";
#elif defined(Q_OS_MAC)
			cmd += ",hwdec=vda";
#endif
			cmd += ",hwdec-codecs=\"" + hwaccCodecs + '"';
		}
		cmd += ",cache=" + (cache > 0 ? QByteArray::number(cache) : "no");
		if (resume > 0)
			cmd += ",start=" + QByteArray::number(resume/1000.0);
		cmd += ",volume=" + QByteArray::number(mpVolume());
		cmd += muted ? ",mute=yes" : ",mute=no";
		cmd += ",audio-delay=" + QByteArray::number(audioSync/1000.0);
		cmd += ",sub-delay=" + QByteArray::number(subDelay/1000.0);
		cmd += ",cache-pause=" + (cacheForPlayback > 0 ? QByteArray::number((int)(cacheForPlayback*0.5)) : "no");
		cmd += ",cache-min=" + QByteArray::number(cacheForPlayback);
		cmd += ",cache-seek-min=" + QByteArray::number(cacheForSeeking);
		cmd += (p->isPaused() || hasImage) ? ",pause=yes" : ",pause=no";
		_Debug("Call: %%", cmd);
		tellmpv(cmd);
	}
	void loadfile() {
		if (startInfo.isValid())
			loadfile(startInfo.mrl.toString().toLocal8Bit(), startInfo.resume, startInfo.cache);
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
				if (!hwaccCodecs.contains(codec))
					return HardwareAcceleration::Deactivated;
			}
		}
		return HardwareAcceleration::Unavailable;
	}
};

PlayEngine::PlayEngine()
: d(new Data(this)) {
	_Debug("Create audio/video plugins.");
	d->audio = new AudioController(this);
	d->video = new VideoOutput(this);

	d->chapterInfo = new ChapterInfoObject(this, this);
	d->audioTrackInfo = new AudioTrackInfoObject(this, this);
	d->imageTicker.setInterval(20);
	d->updateMediaName();

	_Debug("Make registrations and connections");
	static auto stageNotifier = [] (MPContext *mpctx, int stage) {
		static_cast<PlayEngine*>(mpctx->priv)->onMpvStageChanged(stage);
	};
	mp_register_player_stage_notifier(stageNotifier);
	mp_register_player_command_filter(Data::mpCommandFilter);

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
	connect(d->video, &VideoOutput::formatChanged, [this] (const VideoFormat &format) {
		_PostEvent(this, VideoFormatChanged, format);
		_PostEvent(this, HwAccChanged, d->getHwAcc());
	});
	connect(d->video, &VideoOutput::hwAccChanged, [this] () {
		_PostEvent(this, HwAccChanged, d->getHwAcc());
	});
	connect(d->audio, &AudioController::started, [this] () {
		_PostEvent(this, UpdateAudioFormat);
	});

	d->handle = mpv_create();
	d->mpctx = d->handle->mpctx;
	d->mpctx->priv = this;
	mpv_set_event_filter_callback(d->handle, mpvEventFilter, this);
	mpv_request_event(d->handle, MPV_EVENT_TICK, true);
//	mpv_request_log_messages(d->handle, "v");
	d->setOption("fs", "no");
	d->setOption("mouse-movements", "yes");
	d->setOption("af", "dummy:address=" + QByteArray::number((quint64)(quintptr)(void*)(d->audio)));
	d->setOption("vo", "null:address="  + QByteArray::number((quint64)(quintptr)(void*)(d->video)));
	d->setOption("softvol", "yes");
	d->setOption("softvol-max", "1000.0");
	d->setOption("fixed-vo", "yes");
	d->setOption("autosub", "no");
	d->setOption("osd-level", "0");
	d->setOption("quiet", "yes");
	d->setOption("consolecontrols", "no");
	d->setOption("subcp", "utf8");
	d->setOption("ao", "null,");
	d->setOption("ad-lavc-downmix", "no");
	d->setOption("channels", "3");

	auto overrides = qgetenv("CMPLAYER_MPV_OPTIONS").trimmed();
	if (!overrides.isEmpty()) {
		const auto args = QString::fromLocal8Bit(overrides).split(QRegularExpression(R"([\s\t]+)"), QString::SkipEmptyParts);
		for (int i=0; i<args.size(); ++i) {
			if (!args[i].startsWith("--")) {
				_Error("Cannot parse option %%.", args[i]);
				continue;
			}
			const auto arg = args[i].midRef(2);
			const int index = arg.indexOf('=');
			if (index < 0) {
				if (arg.startsWith("no-"))
					d->setOption(arg.mid(3).toLatin1(), "no");
				else
					d->setOption(arg.toLatin1(), "yes");
			} else
				d->setOption(arg.left(index).toLatin1(), arg.mid(index+1).toLatin1());
		}
	}
	d->fatal(mpv_initialize(d->handle), "Couldn't initialize mpv.");
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

void PlayEngine::setSubtitleDelay(int ms) {
	if (_Change(d->subDelay, ms))
		d->setmp("sub-delay", (float)(d->subDelay/1000.0f));
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

const StreamList &PlayEngine::subtitleStreams() const {return d->streams[Stream::Subtitle];}

VideoRendererItem *PlayEngine::videoRenderer() const {return d->renderer;}

const StreamList &PlayEngine::videoStreams() const {return d->streams[Stream::Video];}

int PlayEngine::audioSync() const {return d->audioSync;}
const StreamList &PlayEngine::audioStreams() const {return d->streams[Stream::Audio];}

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
		d->setmpv("speed", speed);
		emit speedChanged(d->speed);
	}
}

void PlayEngine::seek(int pos) {
	d->chapter = -1;
	if (d->hasImage)
		d->image.seek(pos, false);
	else
		d->tellmpv("seek", (double)pos/1000.0, 2);
}

void PlayEngine::relativeSeek(int pos) {
	if (d->hasImage)
		d->image.seek(pos, true);
	else
		d->tellmpv("seek", (double)pos/1000.0, 0);
	emit sought();
}

void PlayEngine::setClippingMethod(ClippingMethod method) {
	d->audio->setClippingMethod(method);
}

void PlayEngine::setChannelLayoutMap(const ChannelLayoutMap &map) {
	d->audio->setChannelLayoutMap(map);
}

void PlayEngine::setChannelLayout(ChannelLayout layout) {
	if (_Change(d->layout, layout) && d->position > 0)
		d->loadfile(d->startInfo.mrl.toString().toLocal8Bit(), d->position, d->startInfo.cache);
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
	d->cacheForPlayback = playback;
	d->cacheForSeeking = seeking;
}

double PlayEngine::volumeNormalizer() const {
	auto gain = d->audio->gain(); return gain < 0 ? 1.0 : gain;
}

void PlayEngine::setHwAccCodecs(const QList<int> &codecs) {
	d->hwaccCodecs.clear();
	for (auto id : codecs) {
		if (const char *name = HwAcc::codecName(id)) {
			d->hwaccCodecs.append(name);
			d->hwaccCodecs.append(',');
		}
	}
	d->hwaccCodecs.chop(1);
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
	return d->streamIds[Stream::Subtitle];
}

void PlayEngine::addSubtitleStream(const QString &fileName, const QString &enc) {
	QFileInfo info(fileName);
	if (info.exists()) {
		SubtitleFileInfo file;
		file.path = info.absoluteFilePath();
		file.encoding = enc;
		d->subtitleFiles.append(file);
		d->tellmpv("sub_add", doubleQuoted(file.path), enc);
	}
}

void PlayEngine::removeSubtitleStream(int id) {
	auto &streams = d->streams[Stream::Subtitle];
	auto it = streams.find(id);
	if (it != streams.end()) {
		if (it->isExternal()) {
			for (int i=0; i<d->subtitleFiles.size(); ++i) {
				if (d->subtitleFiles[i].path == it->m_fileName)
					d->subtitleFiles.removeAt(i);
			}
		}
		d->tellmpv("sub_remove", id);
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

void PlayEngine::setNextStartInfo(const StartInfo &startInfo) {
	d->nextInfo = startInfo;
}

void PlayEngine::updateState(State state) {
	const bool wasRunning = isRunning();
	if (_Change(m_state, state)) {
		emit stateChanged(m_state);
		if (m_state & (Playing | Paused)) {
			if (d->hasImage)
				d->imageTicker.start();
		} else {
			if (d->hasImage)
				d->imageTicker.stop();
		}
		if (wasRunning != isRunning())
			emit runningChanged();
	}
	if (m_state != Paused)
		d->userPaused = false;
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
			if (!d->chapters.isEmpty()) {
				Chapter prev; prev.m_id = -1; prev.m_time = _Min<int>();
				Chapter last; last.m_id = d->chapters.last().id() + 1; last.m_time = _Max<int>();
				d->chapterFakeList.append(prev);
				d->chapterFakeList += d->chapters;
				d->chapterFakeList.append(last);
			} else
				d->chapterFakeList.clear();
		}
		break;
	} case UpdateDVDInfo:
		d->dvd = _GetData<DvdInfo>(event);
		emit dvdInfoChanged();
		break;
	case UpdateCache:
		d->cache = _GetData<int>(event)/(qreal)d->startInfo.cache;
		emit cacheChanged();
		break;
	case UpdateAudioFormat:
		d->audioInfo.setAudio(this);
		emit audioChanged();
		break;
	case UpdateCurrentStream: {
		const auto ids = _GetData<QVector<int>>(event);
		Q_ASSERT(ids.size() == 3);
		auto check = [&] (Stream::Type type, void (PlayEngine::*sig)(int)) {
			const int id = ids[type];
			if (id == d->streamIds[type])
				return;
			auto &streams = d->streams[type];
			for (auto it = streams.begin(); it != streams.end(); ++it)
				it->m_selected = it->id() == id;
			emit (this->*sig)(id);
		};
		check(Stream::Audio, &PlayEngine::currentAudioStreamChanged);
		check(Stream::Video, &PlayEngine::currentVideoStreamChanged);
		check(Stream::Subtitle, &PlayEngine::currentSubtitleStreamChanged);
		break;
	} case UpdateTrackList: {
		auto streams = _GetData<QVector<StreamList>>(event);
		Q_ASSERT(streams.size() == 3);
		auto check = [&] (Stream::Type type, void (PlayEngine::*sig)(const StreamList&)) {
			auto &_streams = d->streams[type];
			if (!_CheckSwap(_streams, streams[type]))
				return false;
			d->streamIds[type] = -1;
			for (auto it = _streams.begin(); it != _streams.end(); ++it) {
				if (it->isSelected()) {
					d->streamIds[type] = it->id();
					break;
				}
			}
			emit (this->*sig)(_streams);
			return true;
		};
		if (check(Stream::Video, &PlayEngine::videoStreamsChanged))
			emit hasVideoChanged();
		if (check(Stream::Audio, &PlayEngine::audioStreamsChanged)) {
			d->audioTrackInfo->setCount(d->streams[Stream::Audio].size());
			d->audioTrackInfo->setCurrent(d->streamIds[Stream::Audio]);
		}
		check(Stream::Subtitle, &PlayEngine::subtitleStreamsChanged);
		break;
	} case StateChange:
		updateState(_GetData<PlayEngine::State>(event));
		break;
	case PreparePlayback: {
		d->subtitleFiles.clear();
		break;
	} case StartPlayback: {
		if (d->renderer)
			d->renderer->reset();
		emit seekableChanged(isSeekable());
		emit mediaChanged();
		emit audioChanged();
		emit cacheChanged();
		updateState(Playing);
		emit started(d->startInfo.mrl);
		break;
	} case EndPlayback: {
		Mrl mrl; bool error; _GetAllData(event, mrl, error);
		const int remain = (d->duration + d->begin) - d->position;
		const bool eof = remain <= 500;
		d->nextInfo = StartInfo();
		if (!error && eof)
			emit requestNextStartInfo();
		updateState(error ? Error : (d->nextInfo.isValid() ? Loading : Stopped));
		if (!error && !mrl.isEmpty())
			emit finished(mrl, d->position, remain);
		if (d->nextInfo.isValid())
			load(d->nextInfo);
		break;
	} case UpdateTimeRange:
		_GetAllData(event, d->begin, d->duration);
		emit durationChanged(d->duration);
		emit beginChanged(d->begin);
		emit endChanged(end());
		break;
	case Tick: {
		d->position = _GetData<int>(event);
		emit tick(d->position);
		emit relativePositionChanged();
		d->tick = false;
		auto findChapterIn = [&] (int begin, int end) {
			end = qMin(end, d->chapterFakeList.size());
			begin = qMax(0, begin);
			for (int i=begin; i<end-1; ++i) {
				if (d->chapterFakeList[i].time() <= d->position && d->position < d->chapterFakeList[i+1].time())
					return d->chapterFakeList[i].id();
			}
			return -2;
		};
		int chapter = -2;
		if (d->chapterFakeList.isEmpty())
			chapter = -2;
		else if (d->chapter < -1)
			chapter = findChapterIn(0, d->chapterFakeList.size());
		else {
			chapter = findChapterIn(d->chapter+1, d->chapterFakeList.size());
			if (chapter == -2)
				chapter = findChapterIn(0, d->chapter+1);
		}
		if (_Change(d->chapter, chapter))
			emit currentChapterChanged(d->chapter);
		break;
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
	auto mrl = d->startInfo.mrl;
	if (mrl.isDvd()) {
		const QString path = "dvdnav://" % QString::number(id) % mrl.location().mid(6);
		d->loadfile(path.toLocal8Bit(), 0, d->startInfo.cache);
	}
}

void PlayEngine::sendDVDCommand(DVDCmd cmd) {
	if (!d->startInfo.mrl.isDvd())
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
	case MP_STAGE_PREPARE_TO_OPEN: {
		d->clear();
		d->video->output(QImage());
		setState(PlayEngine::Loading);
		break;
	} case MP_STAGE_OPEN_DEMUXER: {
		DvdInfo dvd;
		if (d->mpctx->stream && d->mpctx->stream->info && !strcmp(d->mpctx->stream->info->name, "dvd")) {
//			char buffer[256];
//			if (dvd_volume_id(d->mpctx->stream, buffer, sizeof(buffer)))
//				dvd.volume = QString::fromLocal8Bit(buffer);
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
		break;
	} default:
		break;
	}
}


void PlayEngine::setVolume(int volume) {
	if (_Change(d->volume, qBound(0, volume, 100))) {
		d->setmpv("volume", d->mpVolume());
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
		d->setmpv("volume", d->mpVolume());
		emit preampChanged(d->amp);
	}
}

void PlayEngine::setMuted(bool muted) {
	if (_Change(d->muted, muted)) {
		d->tellmpv("mute", (int)d->muted);
		emit mutedChanged(d->muted);
	}
}

int PlayEngine::mpvEventFilter(mpv_event *event, void *ctx) {
	auto e = static_cast<PlayEngine*>(ctx);
	auto d = e->d;
	switch (event->event_id) {
	case MPV_EVENT_START_FILE: {
		d->video->setDeintOptions(d->deint_swdec, d->deint_hwdec);
		d->video->setDeintEnabled(d->deint != DeintMode::None);
		d->audio->setOutputChannelLayout(d->layout);
		return false;
	} default:
		return false;
	}
}

void PlayEngine::exec() {
	auto mpctx = d->mpctx;
	d->init = true;
	d->quit = false;
	int begin = 0, duration = 0, position = 0, cache = -1;
	bool error = true;
	Mrl mrl;
	QRegularExpression regList(_L(R"(((^\d+): )?([^=]+)=(.+)$)"));
	while (!d->quit) {
		const auto event = mpv_wait_event(d->handle, 10000);
		switch (event->event_id) {
		case MPV_EVENT_TICK: {
			auto data = static_cast<const double*>(event->data);
			if (_Change<int>(begin, data[1]*1000 + 0.5) | _Change<int>(duration, data[2]*1000 + 0.5)) {
				_PostEvent(this, UpdateTimeRange, begin, duration);
				auto data = mpv_get_property_string(d->handle, "chapter-list");
				const auto str = QString::fromLocal8Bit(data);
				mpv_free(data);
				if (!str.isEmpty()) {
					const auto lines = str.split('\n', QString::KeepEmptyParts);
					ChapterList chapters; chapters.reserve(lines.size()/3);
					Chapter chapter;
					for (auto &line : lines) {
						if (line.isEmpty()) {
							if (chapter.id() >= -1) {
								if (chapter.m_name.isEmpty())
									chapter.m_name = _MSecToString(chapter.m_time, _L("hh:mm:ss.zzz"));
								chapters.append(chapter);
								chapter = Chapter();
							}
						} else {
							auto match = regList.match(line);
							if (match.hasMatch()) {
								if (!match.capturedRef(2).isEmpty())
									chapter.m_id = match.capturedRef(2).toInt();
								const auto key = match.capturedRef(3);
								const auto value = match.capturedRef(4);
								if (key == _L("title"))
									chapter.m_name = value.toString();
								else if (key == _L("time"))
									chapter.m_time = value.toDouble()*1000 + 0.5;
							}
						}
					}
					_PostEvent(this, UpdateChapterList, chapters);
				}
			}
			if (!d->tick && _Change<int>(position, data[0]*1000 + 0.5)) {
				d->tick = true;
				_PostEvent(this, Tick, position);
			}
			const auto arg = mpv_get_property_string(d->handle, "cache");
			int newCache = -1;
			if (arg) {
				newCache = atoi(arg);
				mpv_free(arg);
			}
			if (_Change(cache, newCache))
				_PostEvent(this, UpdateCache, cache);
			break;
		} case MPV_EVENT_LOG_MESSAGE: {
			auto message = static_cast<mpv_event_log_message*>(event->data);
			qDebug() << QString::fromLocal8Bit(message->text);
			break;
		} case MPV_EVENT_IDLE:
			break;
		case MPV_EVENT_START_FILE:
			error = true;
			position = d->position;
			begin = d->begin;
			duration = d->duration;
			cache = d->cache < 0 ? -1 : d->cache*d->startInfo.cache;
			mrl = d->startInfo.mrl;
			_PostEvent(this, StateChange, Loading);
			_PostEvent(this, PreparePlayback);
			break;
		case MPV_EVENT_PLAYBACK_START: {
			error = false;
			_PostEvent(this, StartPlayback);
			auto data = mpv_get_property_string(d->handle, "media-title");
			qDebug() << "media-title" << data;
			mpv_free(data);
			break;
		} case MPV_EVENT_END_FILE: {
			_PostEvent(this, EndPlayback, mrl, error);
			break;
		} case MPV_EVENT_TRACKS_CHANGED: {
			QVector<StreamList> streams(3);
			auto data = mpv_get_property_string(d->handle, "track-list");
			const auto str = QString::fromLocal8Bit(data);
			mpv_free(data);
			if (!str.isEmpty()) {
				const auto lines = str.split('\n', QString::KeepEmptyParts);
				Stream stream;
				QRegularExpression id(_L(R"(^\d+: id=(\d+)$)"));
				QRegularExpression item(_L(R"(^([^=]+)=(.+)$)"));
				QString titles[3];
				titles[Stream::Audio] = tr("Audio %1");
				titles[Stream::Video] = tr("Video %1");
				titles[Stream::Subtitle] = tr("Subtitle %1");
				for (auto &line : lines) {
					if (line.isEmpty()) {
						if (stream.id() >= 0 && stream.type() != Stream::Unknown) {
							if (stream.title().isEmpty())
								stream.m_title = titles[stream.type()].arg(stream.id());
							streams[stream.type()].insert(stream.id(), stream);
							stream = Stream();
						}
					} else if (stream.id() < 0) {
						auto match = id.match(line);
						if (match.hasMatch())
							stream.m_id = match.capturedRef(1).toInt();
					} else {
						auto match = item.match(line);
						if (match.hasMatch()) {
							const auto key = match.capturedRef(1);
							const auto value = match.capturedRef(2);
							if (key == _L("type")) {
								if (value == _L("audio"))
									stream.m_type = Stream::Audio;
								else if (value == _L("video"))
									stream.m_type = Stream::Video;
								else if (value == _L("sub"))
									stream.m_type = Stream::Subtitle;
								else
									_Error("Cannot parse %%=%% in track list.", key, value);
							} else if (key == _L("title"))
								stream.m_title = value.toString();
							else if (key == _L("lang"))
								stream.m_lang = value.toString();
							else if (key == _L("external-filename")) {
								stream.m_fileName = value.toString();
								stream.m_title = QFileInfo(stream.m_fileName).fileName();
							} else if (key == _L("selected"))
								stream.m_selected = value == _L("yes");
							else if (key == _L("codec"))
								stream.m_codec = value.toString();
							else if (key == _L("default"))
								stream.m_default = value == _L("yes");
						}
					}
				}
			}
			_PostEvent(this, UpdateTrackList, streams);
			break;
		} case MPV_EVENT_TRACK_SWITCHED: {
			auto get = [this] (const char *name) {
				const auto data = mpv_get_property_string(d->handle, name);
				const int id = std::atoi(data); mpv_free(data); return id;
			};
			QVector<int> ids(3, -1);
			ids[Stream::Video] = get("vid");
			ids[Stream::Audio] = get("aid");
			ids[Stream::Subtitle] = get("sid");
			_PostEvent(this, UpdateCurrentStream, ids);
			break;
		} case MPV_EVENT_PAUSE:
			_PostEvent(this, StateChange, d->userPaused ? Paused : Loading);
			break;
		case MPV_EVENT_UNPAUSE:
			_PostEvent(this, StateChange, Playing);
			break;
		case MPV_EVENT_SHUTDOWN:
			goto shutdown;
		default:
			break;
		}
	}
shutdown:
	d->hasImage = false;
	qDebug() << "terminate loop";
	mpctx->opts->hwdec_codecs = nullptr;
	mpctx->opts->hwdec_api = HWDEC_NONE;
	mpv_destroy(d->handle);
	d->mpctx = nullptr;
	d->init = false;
	qDebug() << "terminate engine";
}

void PlayEngine::quit() {
	d->tellmpv("quit 1");
}

const StartInfo &PlayEngine::startInfo() const {
	return d->startInfo;
}

void PlayEngine::load(const StartInfo &info) {
	const bool changed = d->startInfo.mrl != info.mrl;
	d->startInfo = info;
	if (changed)
		d->updateMrl();
	if (info.isValid())
		d->loadfile();
}

int PlayEngine::time() const {
	return d->position;
}

bool PlayEngine::isSeekable() const {
	return d->mpctx && d->mpctx->stream && d->mpctx->stream->seek && (!d->mpctx->demuxer || d->mpctx->demuxer->seekable);
}

bool PlayEngine::hasVideo() const {
	return !d->streams[Stream::Video].isEmpty();
}

int PlayEngine::currentChapter() const {
	return d->chapter;
}

void PlayEngine::pause() {
	if (d->hasImage)
		setState(PlayEngine::Paused);
	else {
		d->userPaused = true;
		d->tellmpv("pause", 1);
	}
}

void PlayEngine::unpause() {
	if (d->hasImage)
		setState(PlayEngine::Playing);
	else {
		d->userPaused = false;
		d->tellmpv("pause", 0);
	}
}

Mrl PlayEngine::mrl() const {
	return d->startInfo.mrl;
}

int PlayEngine::currentAudioStream() const {
	return d->streamIds[Stream::Audio];
}

void PlayEngine::setCurrentVideoStream(int id) {
	d->setmp("video", id);
}

int PlayEngine::currentVideoStream() const {
	return hasVideo() ? d->streamIds[Stream::Video] : -1;
}

void PlayEngine::setCurrentAudioStream(int id) {
	d->setmp("audio", id);
}

void PlayEngine::setAudioSync(int sync) {
	if (_Change(d->audioSync, sync))
		d->setmp("audio-delay", (float)(sync*0.001));
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
	d->tellmpv("stop");
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
	return d->subtitleFiles;
}
