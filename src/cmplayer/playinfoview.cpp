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

extern "C" {
#include <libmpdemux/stheader.h>
#include <codec-cfg.h>
#include <libmpcodecs/vd.h>
#include <libvo/video_out.h>
#include <libao2/audio_out.h>
#include <libaf/af.h>
}

enum PlayInfoViewOsd {
	MediaName,
	RealTimeInfo,
	VideoCodec,
	AudioCodec,
	OsdCount
};

struct PlayInfoView::Data {
	TextOsdRenderer *osds[OsdCount];
	sigar_t *sigar = nullptr;
	QTimer timer;
	const PlayEngine *engine = nullptr;
	const VideoRenderer *video = nullptr;
	const AudioController *audio = nullptr;
//	TextOsdRenderer osd = {Qt::AlignTop | Qt::AlignLeft};
	int time = -1;
	double sync = 0;
	bool visible = false;
	QString vc = {"--"};
	QString size = {"--"}, media = {"--"}, cpu = {"--"}, mem = {"--"};
	QString ac = {"--"};
	QString rinfo = {"--"};
//	TextOsdRenderer osds[OsdCount];
	void show(int idx, const QString &string) {
		osds[idx]->show(string, -1);
	}
	static QString codecInfo (const QString &name, codecs *codec) {
		return name % _L(": ") % _8(codec->info) % _L("<br>");
	}
};

PlayInfoView::PlayInfoView(const PlayEngine *engine, const AudioController *audio, const VideoRenderer *video)
: d(new Data) {
	sigar_open(&d->sigar);
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

	connect(d->engine, SIGNAL(tick(int)), this, SLOT(onTick(int)));
	connect(d->video, SIGNAL(formatChanged(VideoFormat)), this, SLOT(onVideoFormatChanged(VideoFormat)));
	connect(d->audio, SIGNAL(formatChanged(AudioFormat)), this, SLOT(setAudioFormat(AudioFormat)));
	connect(d->engine, SIGNAL(aboutToPlay()), this, SLOT(onAboutToPlay()));
}

PlayInfoView::~PlayInfoView() {
	sigar_close(d->sigar);
	delete d;
}

OsdRenderer &PlayInfoView::osd() const {
	return *d->osds[0];
}

void PlayInfoView::onAboutToPlay() {
	d->ac.clear();
	d->media.clear();
	d->media = _L("<p>") % d->engine->mediaName() % _L("</p>");
	MPContext *mpctx = d->engine->context();
	if (mpctx && mpctx->sh_audio && mpctx->ao) {
		auto sh = mpctx->sh_audio;
		auto ao = mpctx->ao;
		d->ac = _L("<p>&nbsp;<br>") % d->codecInfo(tr("Audio codec"), sh->codec)
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
	}
	d->time = 0;
	d->sync = 0;
	if (d->visible) {
		d->show(MediaName, d->media);
		d->show(AudioCodec, d->ac);
	}
}

void PlayInfoView::setVisible(bool visible) {
	if (d->visible != visible) {
		d->visible = visible;
		if (visible) {
			d->show(MediaName, d->media);
			d->show(VideoCodec, d->vc);
			d->show(AudioCodec, d->ac);
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

void PlayInfoView::onTick(int pos) {
	if (!d->visible)
		return;
	if (d->time == pos/1000)
		return;
	d->time = pos/1000;
	MPContext *mpctx = d->engine->context();
	if (!mpctx)
		return;
	sigar_proc_cpu_t cpu;	sigar_proc_mem_t mem;
	const auto pid = sigar_pid_get(d->sigar);
	sigar_proc_cpu_get(d->sigar, pid, &cpu);
	sigar_proc_mem_get(d->sigar, pid, &mem);
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

	const double fps = d->video->outputFrameRate();
	const int duration = d->engine->duration();
	const double percent = (double)pos/duration*1e2;
	d->rinfo = _L("<p>") % tr("CPU") % _L(": ") % _n(cpu.percent*100.0, 1) % _L("% ")
			% tr("RAM") % _L(": ") % _n((double)mem.resident/(1024*1024), 1) % _L("MB<br>")
			% tr("Time") % _L(": ") % secToString(d->time) % _L("/") % msecToString(duration) % _L("(") % _n(percent, 1) % _L("%)<br>")
			% sync
			% tr("Average frame rate") % _L(": ") % _n(fps, 3) % _L("fps ") % _n(d->video->format().bps(fps)/(1024.0*1024.0), 1) % _L("Mbps")
			% normalized
			% _L("</p>");
	if (d->visible)
		d->show(RealTimeInfo, d->rinfo);
}

void PlayInfoView::onVideoFormatChanged(const VideoFormat &vfmt) {
	d->vc.clear();
	MPContext *mpctx = d->engine->context();
	if (mpctx && mpctx->sh_video) {
		sh_video *sh = mpctx->sh_video;
		d->vc = _L("<p>&nbsp;<br>") % d->codecInfo(tr("Video codec"), sh->codec)
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
	}
	if (d->visible)
		d->show(VideoCodec, d->vc);
}

void PlayInfoView::setAudioFormat(const AudioFormat &afmt) {
//	d->afmt = afmt;
}
