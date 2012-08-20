#include "playinfoview.hpp"
#include "osdstyle.hpp"
#include "playengine.hpp"
#include "audiocontroller.hpp"
#include "avmisc.hpp"
#include "videorenderer.hpp"
#include "textosdrenderer.hpp"
#include "app.hpp"
#include "mpcore.hpp"
#include <stdio.h>
#include <sigar.h>
#include <libavcodec/avcodec.h>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <fcntl.h>
#include <unistd.h>
#include "skin.hpp"

extern "C" {
#include <libmpdemux/stheader.h>
#include <codec-cfg.h>
#include <libmpcodecs/vd.h>
#include <libvo/video_out.h>
#include <libao2/audio_out.h>
#include <libaf/af.h>
#include <osdep/timer.h>
}

enum PlayInfoViewOsd {
	MediaName,
	TimeInfo,
	ResourceUsage,
	VideoCodec,
	AudioCodec,
	OsdCount
};


	typedef QLatin1String _L;

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

struct PlayInfoView::Data {
	ProcStat stat;
	qint64 pid = QCoreApplication::applicationPid();
	sigar_t *sigar = nullptr;
	sigar_mem_t mem;

	const PlayEngine *engine = nullptr;
	const VideoRenderer *video = nullptr;
	const AudioController *audio = nullptr;

	TextOsdRenderer *osds[OsdCount];
	State state = State::Stopped;
	quint64 procCpuTime = 0, totalCpuTime = 0, frameTime = 0, drawnFrames = 0;
	double cpuUsage = 0.0, sync = 0.0;
	QTimer timer;
	int time = -1;
	bool visible = false;
	void show(int idx, const QString &string) {osds[idx]->show(string, -1);}
	static QString codecInfo (const QString &n, codecs *c) {return n % _L(": ") % _8(c->info) % _L("<br>");}
	quint64 getProcCpuTime() {return stat.readProcStat() ? (stat.utime + stat.stime) : 0;}
	quint64 getTotalCpuTime() {return stat.readStat() ? (stat.user + stat.nice + stat.system + stat.idle) : 0;}
};

PlayInfoView::PlayInfoView(const PlayEngine *engine, const AudioController *audio, const VideoRenderer *video)
: d(new Data) {
	sigar_open(&d->sigar);
	sigar_mem_get(d->sigar, &d->mem);

	d->engine = engine;
	d->audio = audio;
	d->video = video;
	for (int i=0; i<OsdCount; ++i) {
		d->osds[i] = new TextOsdRenderer{Qt::AlignTop | Qt::AlignLeft};
		if (i > 0)
			d->osds[i-1]->chaining(d->osds[i]);
	}
	auto style = d->osds[0]->style();
	style.color = Qt::yellow;
	style.size = 0.02;
#ifdef Q_WS_MAC
	style.font.setFamily("monaco");
#elif defined(Q_WS_X11)
	style.font.setFamily("monospace");
#endif
	for (int i=0; i<OsdCount; ++i)
		d->osds[i]->setStyle(style);
	d->timer.setInterval(1000);
	connect(d->engine, SIGNAL(tick(int)), this, SLOT(onTick(int)));
	connect(d->video, SIGNAL(formatChanged(VideoFormat)), this, SLOT(onVideoFormatChanged(VideoFormat)));
	connect(d->audio, SIGNAL(formatChanged(AudioFormat)), this, SLOT(setAudioFormat(AudioFormat)));
	connect(d->engine, SIGNAL(aboutToPlay()), this, SLOT(onAboutToPlay()));
	connect(&d->timer, SIGNAL(timeout()), this, SLOT(updateResourceUsage()));

	d->procCpuTime = d->getProcCpuTime();
	d->totalCpuTime = d->getTotalCpuTime();
	d->drawnFrames = d->video->drawnFrames();
	d->frameTime = GetTimerMS();
}

PlayInfoView::~PlayInfoView() {
	sigar_close(d->sigar);
	delete d;
}

OsdRenderer &PlayInfoView::osd() const {
	return *d->osds[0];
}

void PlayInfoView::onAboutToPlay() {
	if (!d->visible)
		return;

	const QString media = _L("<p>") % d->engine->mediaName() % _L("</p>");
	d->show(MediaName, media);

	MPContext *mpctx = d->engine->context();
	if (mpctx && mpctx->sh_audio && mpctx->ao) {
		auto sh = mpctx->sh_audio;
		auto ao = mpctx->ao;
		const QString audio = _L("<p>&nbsp;<br>") % d->codecInfo(tr("Audio codec"), sh->codec)
			% tr("Input") % _L(": ") % format(sh->format) % _L(" ") % bps(sh->i_bps) % _L(" ")
			% _n(sh->samplerate/1000) % _L("kHz ")
			% _n(sh->channels) % _L("ch ")
			% _n(af_fmt2bits(sh->sample_format)) % _L("bits<br>")
			% tr("Output") % _L(": ") % af_fmt2str_short(mpctx->ao->format) % _L(" ")
			% bps(ao->bps) % _L(" ")
			% _n(ao->samplerate/1000) % _L("kHz ")
			% _n(ao->channels) % _L("ch ")
			% _n(af_fmt2bits(ao->format)) % _L("bits")
			% _L("</p>")
		;
		d->show(AudioCodec, audio);
	}
	d->time = 0;
	d->sync = 0;
}

void PlayInfoView::setVisible(bool visible) {
	if (d->visible != visible) {
		d->visible = visible;
		if (visible) {
			d->timer.start();
			onAboutToPlay();
			onVideoFormatChanged(d->video->format());
			updateResourceUsage();
		} else {
			d->timer.stop();
			for (auto osd : d->osds)
				osd->clear();
		}
	}
}

struct mp_volnorm {
	int method; // method used
	float mul;
};

void PlayInfoView::updateResourceUsage() {
	if (!d->visible)
		return;
	MPContext *mpctx = d->engine->context();
	if (!mpctx)
		return;

	QString normalized, sync;
	if (mpctx->mixer.afilter) {
		auto af = mpctx->mixer.afilter->first;
		while (af) {
			if (strcmp(af->info->name, "volnorm") == 0)
				break;
			af = af->next;
		}
		if (af) {
			auto volnorm = reinterpret_cast<mp_volnorm*>(af->setup);
			const double multiplied = volnorm->mul*100.0;
			normalized = _L("<br>") % tr("Volum normalizer") % _L(": ") % _n(multiplied, 1) % _L("%");
		}
	}
	if (mpctx->sh_audio && mpctx->sh_video) {
		const int diff = (mpctx->total_avsync_change - d->sync)*1000000 + 0.5;
		sync = tr("Average A-V sync") % _L(": ") % (diff < 0 ? _L("") : _L("+")) % _n(diff) % _L("us<br>");
		d->sync = mpctx->total_avsync_change;
	}

	const auto procCpuTime = d->getProcCpuTime();
	const auto totalCpuTime = d->getTotalCpuTime();
	if (procCpuTime > d->procCpuTime && totalCpuTime > d->totalCpuTime) {
		d->cpuUsage = double(procCpuTime - d->procCpuTime)/double(totalCpuTime - d->totalCpuTime)*100.0;
		d->procCpuTime = procCpuTime;
		d->totalCpuTime = totalCpuTime;
	}
	const auto frames = d->video->drawnFrames();
	const auto frameTime = GetTimerMS();
	double fps = 0.0;
	if (frameTime > d->frameTime) {
		fps = double(frames - d->drawnFrames)*1000.0/(frameTime - d->frameTime);
		if (fps < 0.0)
			fps = 0.0;
		d->drawnFrames = frames;
		d->frameTime = frameTime;
	}
	sigar_proc_mem_t mem;
	sigar_proc_mem_get(d->sigar, d->pid, &mem);
	const double usingRam = (double)mem.resident/(1024*1024);
	const double totalRam = (double)d->mem.total/(1024*1024);
	const QString info = _L("<p>&nbsp;<br>")
			% tr("CPU usage") % _L(": ") % _n(d->cpuUsage, 1) % _L("%(") % tr("average per-core") % _L(")<br>")
			% tr("RAM usage") % _L(": ") % _n(usingRam, 1) % _L("MB(")
				% _n(usingRam*100.0/totalRam, 1) % _L("%)<br>") % sync
			% tr("Average frame rate") % _L(": ") % _n(fps, 3) % _L("fps(") % _n(d->video->format().bps(fps)/(1024.0*1024.0), 1) % _L("Mbps)")
			% normalized % _L("</p>");
	d->show(ResourceUsage, info);
}

void PlayInfoView::onTick(int pos) {
	if (!d->visible)
		return;
	const int time = pos/1000;
	const auto state = d->engine->state();
	if (d->time == time && d->state == state)
		return;
	d->time = time;
	d->state = state;
	MPContext *mpctx = d->engine->context();
	if (!mpctx)
		return;
	const int duration = d->engine->duration();
	const double percent = (double)pos/duration*1e2;
	const QString info = _L("<p>[") % Skin::mediaStateText(d->state) % _L("]")
			% secToString(d->time) % _L("/") % msecToString(duration) % _L("(") % _n(percent, 1) % _L("%)</p>");
	if (d->visible)
		d->show(TimeInfo, info);
}

void PlayInfoView::onVideoFormatChanged(const VideoFormat &vfmt) {
	if (!d->visible)
		return;
	MPContext *mpctx = d->engine->context();
	if (mpctx && mpctx->sh_video) {
		sh_video *sh = mpctx->sh_video;
		const QString video = _L("<p>&nbsp;<br>") % d->codecInfo(tr("Video codec"), sh->codec)
			% tr("Input") % _L(": ") % format(sh->format) % _L(" ")
			% resolution(sh->disp_w, sh->disp_h) % _L(" ")
			% _n(sh->fps, 3) % _L("fps ")
			% bps(mpctx->sh_video->i_bps) % _L("<br>")
			% tr("Output") % _L(": ") % format(vfmt.type) % _L(" ")
			% resolution(d->video->outputWidth(), vfmt.height) % _L(" ")
			% _n(sh->fps, 3) % _L("fps ")
			% _n(d->video->format().bps(sh->fps)/(1024.0*1024.0), 1) % _L("Mbps")
			% _L("</p>")
		;
		d->show(VideoCodec, video);
	}
}

void PlayInfoView::setAudioFormat(const AudioFormat &/*afmt*/) {
//	d->afmt = afmt;
}
