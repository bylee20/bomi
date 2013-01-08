#include "playinfoitem.hpp"
#include "audiocontroller.hpp"
#include "videorendereritem.hpp"
#include "playengine.hpp"
#include "mpcore.hpp"
#include "avmisc.hpp"
#include <sigar.h>

extern "C" {
#include <libmpdemux/stheader.h>
#include <codec-cfg.h>
#include <libmpcodecs/vd.h>
#include <libvo/video_out.h>
#include <libao2/audio_out.h>
#include <libaf/af.h>
#include <osdep/timer.h>
#include <osdep/numcores.h>
}

#ifdef Q_OS_X11
struct ProcStat {
	ProcStat() {
		const QString path = _L("/proc/") + QString::number(QCoreApplication::applicationPid()) + _L("/stat");
		statFilePath = path.toLocal8Bit();
		buffer.resize(BUFSIZ);
	}
	QByteArray buffer;
	QByteArray statFilePath;
	int pid, ppid, pgrp, session, tty_nr, tpgid;
	uint flags;
	unsigned long int minflt, cminflt, majflt, cmajflt, utime, stime;
	char comm[256];
	char state;
	bool readProcStat() {
		const auto fd = open(statFilePath.data(), O_RDONLY);
		if (fd < 0)
			return false;
		int len = ::read(fd, buffer.data(), buffer.size());
		if (len > 0) {
			buffer[len] = '\0';
			len = sscanf(buffer.data()
				, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu"
				, &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid, &flags
				, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime);
		}
		close(fd);
		return len > 0;
	}

	quint64 user, nice, system, idle;

	bool readStat() {
		const auto fd = open("/proc/stat", O_RDONLY);
		if (fd < 0)
			return false;
		int len = -1;
		for (;;) {
			len = ::read(fd, buffer.data(), buffer.size());
			if (len < 0)
				break;
			buffer[len] = '\0';
			const char *str = strstr(buffer.data(), "cpu");
			if (!str)
				continue;
			str += 3;
			len = sscanf(str, "%llu %llu %llu %llu", &user, &nice, &system, &idle);
			break;
		}
		close(fd);
		return len > 0;
	}
};
#endif


void AvInfoObject::setVideo(const VideoFormat &fmt, const PlayEngine *engine) {
	auto mpctx = engine->context();
	if (!mpctx || !mpctx->sh_video)
		return;
	auto sh = mpctx->sh_video;

	m_hwaccel = engine->isHardwareAccelerated();
	qDebug() << "hw acc?" << m_hwaccel;
	m_codec = _u(sh->codec->info);
	m_input->m_type = format(sh->format);
	m_input->m_size = QSize(sh->disp_w, sh->disp_h);
	m_input->m_fps = sh->fps;
	m_input->m_bps = sh->i_bps*8;
	m_output->m_type = format(fmt.type);
	m_output->m_size = fmt.size();
	m_output->m_fps = sh->fps;
	m_output->m_bps = fmt.bps(sh->fps);
}

void AvInfoObject::setAudio(const PlayEngine *engine) {
	auto mpctx = engine->context();
	if (!mpctx || !mpctx->sh_audio || !mpctx->ao)
		return;
	auto sh = mpctx->sh_audio;
	auto ao = mpctx->ao;
	m_hwaccel = false;
	m_codec = _u(sh->codec->info);

	m_input->m_type = format(sh->format);
	m_input->m_bps = sh->i_bps*8;
	m_input->m_samplerate = sh->samplerate/1000.0; // kHz
	m_input->m_channels = sh->channels;
	m_input->m_bits = af_fmt2bits(sh->sample_format);

	m_output->m_type = _u(af_fmt2str_short(ao->format));
	m_output->m_bps = ao->bps*8;
	m_output->m_samplerate = ao->samplerate/1000.0;
	m_output->m_channels = ao->channels;
	m_output->m_bits = af_fmt2bits(ao->format);
}

struct PlayInfoItem::Data {
#ifdef Q_OS_X11
	ProcStat stat;
	quint64 procCpuTime = 0, totalCpuTime = 0;
	quint64 getProcCpuTime() {return stat.readProcStat() ? (stat.utime + stat.stime) : 0;}
	quint64 getTotalCpuTime() {return stat.readStat() ? (stat.user + stat.nice + stat.system + stat.idle) : 0;}
#endif
	qint64 pid = QCoreApplication::applicationPid();
	sigar_t *sigar = nullptr;
	sigar_mem_t mem;
	const int cores = default_thread_count();

	const PlayEngine *engine = nullptr;
	const VideoRendererItem *video = nullptr;
	const AudioController *audio = nullptr;

	Global::State state = Global::State::Stopped;
	quint64 frameTime = 0, drawnFrames = 0;
	double cpuUsage = 0.0, sync = 0.0;
	QTimer timer;
	int time = -1;
};

PlayInfoItem::PlayInfoItem(QQuickItem *parent)
: QQuickItem(parent), d(new Data) {
	sigar_open(&d->sigar);
	sigar_mem_get(d->sigar, &d->mem);
	m_totmem = (double)d->mem.total/(1024.*1024.);
#ifdef Q_OS_X11
	d->procCpuTime = d->getProcCpuTime();
	d->totalCpuTime = d->getTotalCpuTime();
#endif
}

PlayInfoItem::~PlayInfoItem() {
	sigar_close(d->sigar);
	delete d;
}

QString PlayInfoItem::monospace() const {
#ifdef Q_OS_MAC
	return _L("monaco");
#elif defined(Q_OS_X11)
	return _L("monospace");
#endif
}

//void PlayInfoItem::setRunning(bool run) {
//	if (m_running != run) {
//		m_running = run;
//		emit runningChanged(m_running);
//	}
//}

void PlayInfoItem::set(const PlayEngine *engine, const VideoRendererItem *video, const AudioController *audio) {
	d->engine = engine;
	d->video = video;
	d->audio = audio;

	connect(d->engine, &PlayEngine::tick, [this] (int pos) {
		if (isVisible()) {
			setPosition(pos);
			if (d->state != d->engine->state()) {
				d->state = d->engine->state();
				setState((State)(int)d->state);
				setDuration(d->engine->duration());
			}
		}
	});
	connect(d->video, &VideoRendererItem::formatChanged, [this] (const VideoFormat &format) {
		m_video->setVideo(format, d->engine);
		emit videoChanged();
	});
	connect(d->engine, &PlayEngine::aboutToPlay, [this] () {
		m_media->m_name = d->engine->mediaName();
		m_audio->setAudio(d->engine);
		emit audioChanged();
		d->time = 0;
		d->sync = 0;
		emit mediaChanged();
	});
	connect(d->audio, &AudioController::volumeNormalizedChanged, [this] (bool volnorm) {
		m_volnorm = volnorm;
		emit volumeNormalizedChanged(volnorm);
	});
	m_volnorm = d->audio->isVolumeNormalized();
	d->drawnFrames = d->video->drawnFrames();
	d->frameTime = GetTimerMS();
}

void PlayInfoItem::collect() {
	if (!isVisible() || !d->engine || !d->engine->context())
		return;
	if (d->audio)
		m_norm = d->audio->volumeNormalizer();
	auto mpctx = d->engine->context();
	if (mpctx->sh_audio && mpctx->sh_video) {
		m_avSync = (mpctx->total_avsync_change - d->sync)*1000.0;
		d->sync = mpctx->total_avsync_change;
	}

#ifdef Q_OS_MAC
	sigar_proc_cpu_t cpu;
	sigar_proc_cpu_get(d->sigar, d->pid, &cpu);
	m_cpu = cpu.percent*100.0/d->cores;
#endif
#ifdef Q_OS_X11
	const auto procCpuTime = d->getProcCpuTime();
	const auto totalCpuTime = d->getTotalCpuTime();
	if (procCpuTime > d->procCpuTime && totalCpuTime > d->totalCpuTime) {
		m_cpu = double(procCpuTime - d->procCpuTime)/double(totalCpuTime - d->totalCpuTime)*100.0;
		d->procCpuTime = procCpuTime;
		d->totalCpuTime = totalCpuTime;
	}
#endif
	const auto frameTime = GetTimerMS();
	const auto frames = d->video->drawnFrames();
	m_fps = 0.0;
	if (frameTime > d->frameTime) {
		m_fps = double(frames - d->drawnFrames)*1000.0/(frameTime - d->frameTime);
		if (m_fps < 0.0)
			m_fps = 0.0;
		d->drawnFrames = frames;
		d->frameTime = frameTime;
	}
	m_bps = d->video ? d->video->format().bps(m_fps) : 0.0;

	sigar_proc_mem_t mem;
	sigar_proc_mem_get(d->sigar, d->pid, &mem);
	m_mem = (double)mem.resident/(1024.*1024.);
}

QString PlayInfoItem::stateText() const {
	switch (m_state) {
	case Playing:
		return tr("Playing");
	case Stopped:
		return tr("Stopped");
	case Finished:
		return tr("Finished");
	case Buffering:
		return tr("Buffering");
	case Opening:
		return tr("Opening");
	case Error:
		return tr("Error");
	case Preparing:
		return tr("Preparing");
	default:
		return tr("Paused");
	}
}
