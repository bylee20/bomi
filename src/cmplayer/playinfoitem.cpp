//#include "playinfoitem.hpp"
//#include "playlistmodel.hpp"
//#include "videorendereritem.hpp"
//#include "playengine.hpp"
//#include "mpcore.hpp"
//#include "videoformat.hpp"
//#include <sigar.h>

//extern "C" {
//#include <demux/stheader.h>
//#include <core/codec-cfg.h>
//#include <video/decode//vd.h>
//#include <video/out/vo.h>
//#include <audio/out/ao.h>
//#include <audio/filter/af.h>
//#include <osdep/timer.h>
//#include <osdep/numcores.h>
//}

//#ifdef Q_OS_LINUX
//#include <fcntl.h>
//#include <unistd.h>
//struct ProcStat {
//	ProcStat() {
//		const QString path = _L("/proc/") + QString::number(QCoreApplication::applicationPid()) + _L("/stat");
//		statFilePath = path.toLocal8Bit();
//		buffer.resize(BUFSIZ);
//	}
//	QByteArray buffer;
//	QByteArray statFilePath;
//	int pid, ppid, pgrp, session, tty_nr, tpgid;
//	uint flags;
//	unsigned long int minflt, cminflt, majflt, cmajflt, utime, stime;
//	char comm[256];
//	char state;
//	bool readProcStat() {
//		const auto fd = open(statFilePath.data(), O_RDONLY);
//		if (fd < 0)
//			return false;
//		int len = ::read(fd, buffer.data(), buffer.size());
//		if (len > 0) {
//			buffer[len] = '\0';
//			len = sscanf(buffer.data()
//				, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu"
//				, &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags
//				, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime);
//		}
//		close(fd);
//		return len > 0;
//	}

//	quint64 user, nice, system, idle;

//	bool readStat() {
//		const auto fd = open("/proc/stat", O_RDONLY);
//		if (fd < 0)
//			return false;
//		int len = -1;
//		for (;;) {
//			len = ::read(fd, buffer.data(), buffer.size());
//			if (len < 0)
//				break;
//			buffer[len] = '\0';
//			const char *str = strstr(buffer.data(), "cpu");
//			if (!str)
//				continue;
//			str += 3;
//			len = sscanf(str, "%llu %llu %llu %llu", &user, &nice, &system, &idle);
//			break;
//		}
//		close(fd);
//		return len > 0;
//	}
//};
//#endif


//void AvInfoObject::setVideo(const PlayEngine *engine) {
//	auto mpctx = engine->context();
//	if (!mpctx || !mpctx->sh_video || !mpctx->sh_video->codec)
//		return;
//	const auto fmt = engine->videoFormat();
//	auto sh = mpctx->sh_video;

//	m_HwAcc = engine->isHwAccActivated();
//	m_codec = _U(sh->codec->info);
//	m_input->m_type = format(sh->format);
//	m_input->m_size = QSize(sh->disp_w, sh->disp_h);
//	m_input->m_fps = sh->fps;
//	m_input->m_bps = sh->i_bps*8;
//	m_output->m_type = format(fmt.type());
//	m_output->m_size = fmt.size();
//	if (engine->videoAspectRatio() > 0.1)
//		m_output->m_size.rwidth() = fmt.height()*engine->videoAspectRatio();
//	m_output->m_fps = sh->fps;
//	m_output->m_bps = fmt.bps(sh->fps);
//}

//void AvInfoObject::setAudio(const PlayEngine *engine) {
//	auto mpctx = engine->context();
//	if (!mpctx || !mpctx->sh_audio || !mpctx->ao)
//		return;
//	auto sh = mpctx->sh_audio;
//	auto ao = mpctx->ao;
//	m_HwAcc = false;
//	m_codec = _U(sh->codec->info);

//	m_input->m_type = format(sh->format);
//	m_input->m_bps = sh->i_bps*8;
//	m_input->m_samplerate = sh->samplerate/1000.0; // kHz
//	m_input->m_channels = sh->channels;
//	m_input->m_bits = af_fmt2bits(sh->sample_format);

//	m_output->m_type = _U(af_fmt2str_short(ao->format));
//	m_output->m_bps = ao->bps*8;
//	m_output->m_samplerate = ao->samplerate/1000.0;
//	m_output->m_channels = ao->channels;
//	m_output->m_bits = af_fmt2bits(ao->format);
//}

//struct PlayInfoItem::Data {
//#ifdef Q_OS_LINUX
//	ProcStat stat;
//	quint64 procCpuTime = 0, totalCpuTime = 0;
//	quint64 getProcCpuTime() {return stat.readProcStat() ? (stat.utime + stat.stime) : 0;}
//	quint64 getTotalCpuTime() {return stat.readStat() ? (stat.user + stat.nice + stat.system + stat.idle) : 0;}
//#endif
//	qint64 pid = QCoreApplication::applicationPid();
//	sigar_t *sigar = nullptr;
//	sigar_mem_t mem;
//	const int cores = default_thread_count();

//	const PlayEngine *engine = nullptr;

//	quint64 frameTime = 0, drawnFrames = 0;
//	double cpuUsage = 0.0, sync = 0.0;
//	QTimer timer;
//	int time = -1;
//	quint64 getDrawnFrames() {
//		return engine && engine->videoRenderer() ? engine->videoRenderer()->drawnFrames() : 0;
//	}
//};

//PlayInfoItem::PlayInfoItem(QQuickItem *parent)
//: QQuickItem(parent), d(new Data) {
//	sigar_open(&d->sigar);
//	sigar_mem_get(d->sigar, &d->mem);
//	m_totmem = (double)d->mem.total/(1024.*1024.);
//#ifdef Q_OS_LINUX
//	d->procCpuTime = d->getProcCpuTime();
//	d->totalCpuTime = d->getTotalCpuTime();
//#endif
//}

//PlayInfoItem::~PlayInfoItem() {
//	sigar_close(d->sigar);
//	delete d;
//}

//QString PlayInfoItem::monospace() const {
//#ifdef Q_OS_MAC
//	return _L("monaco");
//#elif defined(Q_OS_LINUX)
//	return _L("monospace");
//#endif
//}


//VideoRendererItem *PlayInfoItem::renderer() const {
//	return d->engine ? d->engine->videoRenderer() : nullptr;
//}

//void PlayInfoItem::set(const PlayEngine *engine) {
//	auto fullScreen = window()->windowState() == Qt::WindowFullScreen;
//	if (fullScreen != m_fullScreen)
//		emit fullScreenChanged(m_fullScreen = fullScreen);

//	plug(window(), &QWindow::windowStateChanged, [this] (Qt::WindowState state) {
//		auto fullScreen = state == Qt::WindowFullScreen;
//		if (fullScreen != m_fullScreen)
//			emit fullScreenChanged(m_fullScreen = fullScreen);
//	});

//	d->engine = engine;

//	plug(d->engine, &PlayEngine::tick, [this] (int pos) {
//		if (isVisible()) {
//			setPosition(pos);
//			setState((State)d->engine->state());
//			setDuration(d->engine->duration());
//		}
//	});

//	plug(d->engine, &PlayEngine::videoFormatChanged, [this] () {
//		m_video->setVideo(d->engine);
//		emit videoChanged();
//	});
//	plug(d->engine, &PlayEngine::videoAspectRatioChanged, [this] () {
//		m_video->setVideo(d->engine);
//		emit videoChanged();
//	});
//	plug(d->engine, &PlayEngine::aboutToPlay, [this] () {
//		m_media->m_mrl = d->engine->mrl();
//		m_media->m_name = d->engine->mediaName();
//		m_audio->setAudio(d->engine);
//		emit audioChanged();
//		d->time = 0;
//		d->sync = 0;
//		emit mediaChanged();
//	});
//	plug(d->engine, &PlayEngine::audioFilterChanged, [this] (const QString &af, bool on) {
//		if (af == _L("volnorm"))
//			emit volumeNormalizedChanged(m_volnorm = on);
//	});

//	plug(d->engine, &PlayEngine::volumeChanged, [this](int volume) {
//		emit volumeChanged(m_volume = volume);
//	});
//	plug(d->engine, &PlayEngine::mutedChanged, [this] (bool muted) {
//		emit mutedChanged(m_muted = muted);
//	});

//	if (d->engine->videoRenderer()) {
//		d->drawnFrames = d->getDrawnFrames();
//		d->frameTime = GetTimerMS();
//	}

//	emit mutedChanged(m_muted = d->engine->isMuted());
//	emit durationChanged(m_duration = d->engine->duration());
//	emit tick(m_position = d->engine->position());
//	emit videoChanged();
//	emit audioChanged();
//	emit mediaChanged();
//	emit stateChanged(m_state = (State)d->engine->state());
//	emit volumeNormalizedChanged(m_volnorm = d->engine->hasAudioFilter("volnorm"));
//	emit volumeChanged(m_volume = d->engine->volume());
//	emit fullScreenChanged(m_fullScreen = (window()->windowState() == Qt::WindowFullScreen));
//}

//void PlayInfoItem::collect() {
//	if (!isVisible() || !d->engine || !d->engine->context())
//		return;
//	m_norm = d->engine->volumeNormalizer();
//	auto mpctx = d->engine->context();
//	if (mpctx->sh_audio && mpctx->sh_video) {
//		m_avSync = (mpctx->total_avsync_change - d->sync)*1000.0;
//		d->sync = mpctx->total_avsync_change;
//	}

//#ifdef Q_OS_MAC
//	sigar_proc_cpu_t cpu;
//	sigar_proc_cpu_get(d->sigar, d->pid, &cpu);
//	m_cpu = cpu.percent*100.0/d->cores;
//#endif
//#ifdef Q_OS_LINUX
//	const auto procCpuTime = d->getProcCpuTime();
//	const auto totalCpuTime = d->getTotalCpuTime();
//	if (procCpuTime > d->procCpuTime && totalCpuTime > d->totalCpuTime) {
//		m_cpu = double(procCpuTime - d->procCpuTime)/double(totalCpuTime - d->totalCpuTime)*100.0;
//		d->procCpuTime = procCpuTime;
//		d->totalCpuTime = totalCpuTime;
//	}
//#endif
//	const auto frameTime = GetTimerMS();
//	const auto frames = d->getDrawnFrames();
//	m_fps = 0.0;
//	if (frameTime > d->frameTime) {
//		m_fps = double(frames - d->drawnFrames)*1000.0/(frameTime - d->frameTime);
//		if (m_fps < 0.0)
//			m_fps = 0.0;
//		d->drawnFrames = frames;
//		d->frameTime = frameTime;
//	}
//	m_bps = d->engine->videoFormat().bps(m_fps);

//	sigar_proc_mem_t mem;
//	sigar_proc_mem_get(d->sigar, d->pid, &mem);
//	m_mem = (double)mem.resident/(1024.*1024.);
//}

//QString PlayInfoItem::stateText() const {
//	switch (m_state) {
//	case Playing:
//		return tr("Playing");
//	case Stopped:
//		return tr("Stopped");
//	case Finished:
//		return tr("Finished");
//	case Buffering:
//		return tr("Buffering");
//	case Opening:
//		return tr("Opening");
//	case Error:
//		return tr("Error");
//	case Preparing:
//		return tr("Preparing");
//	default:
//		return tr("Paused");
//	}
//}
