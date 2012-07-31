#include "playinfoview.hpp"
#include "osdstyle.hpp"
#include "richstring.hpp"
#include "playengine.hpp"
#include "audiocontroller.hpp"
#include "avmisc.hpp"
#include "videorenderer.hpp"
#include "textosdrenderer.hpp"
#include "app.hpp"
#include <stdio.h>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <sigar.h>
#include "mpcore.hpp"
#include <libavcodec/avcodec.h>

extern "C" {
#include <libmpdemux/stheader.h>
#include <codec-cfg.h>
#include <libmpcodecs/vd.h>
#include <libvo/video_out.h>
#include <libao2/audio_out.h>
}

struct PlayInfoView::Data {
	sigar_t *sigar = nullptr;
	QTimer timer;
	const PlayEngine *engine = nullptr;
	const VideoRenderer *video = nullptr;
	const AudioController *audio = nullptr;
	TextOsdRenderer osd = {Qt::AlignTop | Qt::AlignLeft};
	bool visible = false;
	QString vinput = {"--"}, voutput = {"--"};
	QString size = {"--"}, vfps = {"--"}, cpu = {"--"}, mem = {"--"};
	QString ainput = {"--"};
};

static inline QString toString(double value, int n = 1) {
	char fmt[10];	sprintf(fmt, "%%.%df", n);
	return value > 0 ? QString().sprintf(fmt, value) : QLatin1String("--");
}

PlayInfoView::PlayInfoView(const PlayEngine *engine, const AudioController *audio, const VideoRenderer *video)
: d(new Data) {
	sigar_open(&d->sigar);
	d->engine = engine;
	d->audio = audio;
	d->video = video;
	OsdStyle style = d->osd.style();
	style.color_fg = QColor(Qt::yellow);
	style.text_scale = 0.02;
#ifdef Q_WS_MAC
	style.font.setFamily("monaco");
#elif defined(Q_WS_X11)
	style.font.setFamily("monospace");
#endif
	d->osd.setStyle(style);

	d->timer.setInterval(2000);
	connect(&d->timer, SIGNAL(timeout()), this, SLOT(update()));
	connect(d->video, SIGNAL(formatChanged(VideoFormat)), this, SLOT(onVideoFormatChanged(VideoFormat)));
	connect(d->audio, SIGNAL(formatChanged(AudioFormat)), this, SLOT(setAudioFormat(AudioFormat)));
	connect(d->engine, SIGNAL(aboutToPlay()), this, SLOT(onAboutToPlay()));
}

PlayInfoView::~PlayInfoView() {
	sigar_close(d->sigar);
	delete d;
}

OsdRenderer &PlayInfoView::osd() const {
	return d->osd;
}

void PlayInfoView::onAboutToPlay() {

	d->vinput.clear();
	d->voutput.clear();
	MPContext *mpctx = d->engine->context();
	if (!mpctx)
		return;

	auto codecInfo = [this] (const QString &name, codecs *codec) -> QString {
		return name % _L(": ") % _8(codec->info) % _L("<br>");
	};

	if (mpctx->sh_video) {
		sh_video *sh = mpctx->sh_video;
		d->vinput = codecInfo(tr("Video codec"), sh->codec)
			% tr("Input") % _L(": ") % format(sh->format) % _L(" ")
			% resolution(sh->disp_w, sh->disp_h) % _L(" ")
			% _n(sh->fps) % _L("fps ")
			% bps(mpctx->sh_video->i_bps)
		;
	}
	if (mpctx->sh_audio && mpctx->ao) {
		auto sh = mpctx->sh_audio;
		auto ao = mpctx->ao;
		d->ainput = codecInfo(tr("Audio codec"), sh->codec)
			% _L("Input : ") % format(sh->format) % _L(" ") % bps(sh->i_bps) % _L("<br>")
			% _L("&nbsp;&nbsp;&nbsp;=>&nbsp;&nbsp;&nbsp;") % af_fmt2str_short(sh->sample_format) % _L(" ")
			% bps(sh->o_bps) % _L(" ")
			% _n(sh->samplerate/1000) % _L("kHz ")
			% _n(sh->channels) % _L("ch ")
			% _n(af_fmt2bits(sh->sample_format)) % _L("bits<br>")
			% _L("Output: ") % af_fmt2str_short(mpctx->ao->format) % _L(" ")
			% bps(ao->bps) % _L(" ")
			% _n(ao->samplerate/1000) % _L("kHz ")
			% _n(ao->channels) % _L("ch ")
			% _n(af_fmt2bits(ao->format)) % _L("bits")
		;
	}
}

void PlayInfoView::setVisible(bool visible) {
	if (d->visible != visible) {
		d->visible = visible;
		if (visible) {
			update();
			d->timer.start();
		} else {
			d->timer.stop();
			d->osd.clear();
			d->cpu = d->vfps = -1;
		}
	}
}

void PlayInfoView::update() {
	sigar_proc_cpu_t cpu;	sigar_proc_mem_t mem;
	const auto pid = sigar_pid_get(d->sigar);
	sigar_proc_cpu_get(d->sigar, pid, &cpu);
	sigar_proc_mem_get(d->sigar, pid, &mem);

	const double fps = d->video->outputFrameRate();
	typedef QLatin1String _L;
	QString text = _L("<p>")
		% d->engine->mediaName() % _L("<br>")
		% tr("CPU usage") % _L(": ") % toString(cpu.percent*100.0, 1) % _L("% ")
		% tr("RAM usage") % _L(": ") % toString((double)mem.resident/(1024*1024), 1) % _L("MB</p>")
		% _L("<p>")
		% d->vinput % _L("<br>")
		% d->voutput
		% _n(fps, 1) % _L("fps ")
		% _n(d->video->format().bps(fps)/1024.0, 1) % _L("kbps")
		% _L("</p><p>")
		% d->ainput
		% _L("</p>");
	if (d->visible)
		d->osd.show(text, -1);
}

void PlayInfoView::onVideoFormatChanged(const VideoFormat &vfmt) {
	d->voutput = tr("Output") % _L(": ") % format(vfmt.fourcc) % _L(" ")
		% resolution(d->video->outputWidth(), vfmt.height) % _L(" ");

}

void PlayInfoView::setAudioFormat(const AudioFormat &afmt) {
//	d->afmt = afmt;
}
