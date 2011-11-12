#include "playinfoview.hpp"
#include "osdstyle.hpp"
#include "richstring.hpp"
#include "audiocontroller.hpp"
#include "avmisc.hpp"
#include "videorenderer.hpp"
#include "textosdrenderer.hpp"
#include "application.hpp"
#include <stdio.h>
#include <QtCore/QTimer>
#include <QtCore/QDebug>

struct PlayInfoView::Data {
	QTimer *timer;
	VideoFormat vfmt;
	AudioFormat afmt;
	const VideoRenderer *video;
	const AudioController *audio;
	QString vfps, cpu, mem;
	TextOsdRenderer *osd;
	bool visible;
	QString vinput, voutput;
	QString size;
};

static inline QString toString(double value, int n = 1) {
	char fmt[10];	sprintf(fmt, "%%.%df", n);
	return value > 0 ? QString().sprintf(fmt, value) : QLatin1String("--");
}

PlayInfoView::PlayInfoView(QObject *parent)
: QObject(parent), d(new Data) {
	d->osd = new TextOsdRenderer(Qt::AlignTop | Qt::AlignLeft);
	OsdStyle style = d->osd->style();
	style.color_fg = QColor(Qt::yellow);
	style.text_scale = 0.02;
	d->osd->setStyle(style);
	d->size = d->mem = d->cpu = d->vfps = QLatin1String("--");
	d->visible = false;
	d->timer = new QTimer(this);
	d->timer->setInterval(500);
	d->video = 0;
	d->vinput = d->voutput = "--";
	d->audio = 0;
	connect(app(), SIGNAL(gotProcInfo(double,int,double)), this, SLOT(setProcInfo(double,int,double)));
	connect(d->timer, SIGNAL(timeout()), this, SLOT(update()));
}

PlayInfoView::~PlayInfoView() {
	delete d->timer;
	delete d;
}

OsdRenderer *PlayInfoView::osd() const {
	return d->osd;
}

void PlayInfoView::setVideo(const VideoRenderer *video) {
	d->video = video;
	connect(d->video, SIGNAL(formatChanged(VideoFormat)), this, SLOT(setVideoFormat(VideoFormat)));
}

void PlayInfoView::setVideoFormat(const VideoFormat &vfmt) {
	d->vfmt = vfmt;
	d->size = QString::number(vfmt.width);
	d->size += QString::fromUtf8("\303\227");
	d->size += QString::number(vfmt.height);
	const double sar = (double)d->vfmt.width/(double)d->vfmt.height;
	const double dar = sar*d->vfmt.sar;
	const QString fmt("fourcc: %1, fps: %3, aspect ratio: %2");
	d->vinput = fmt.arg(VideoFormat::fourccToString(d->vfmt.source_fourcc))
		.arg(toString(sar, 2)).arg(toString(d->vfmt.fps, 2));
	d->voutput = fmt.arg(VideoFormat::fourccToString(d->vfmt.output_fourcc))
		.arg(toString(dar, 2));
	update();
}

void PlayInfoView::setProcInfo(double cpu, int rss, double mem) {
	d->cpu = toString(cpu);
	d->cpu += QLatin1String("%");
	d->mem = toString(rss/1024.0);
	d->mem += QLatin1String("MB (");
	d->mem += toString(mem);
	d->mem += QLatin1String("%)");
}

void PlayInfoView::setVisible(bool visible) {
	if (d->visible != visible) {
		d->visible = visible;
		if (visible) {
			app()->getProcInfo();
			d->timer->start();
		} else {
			d->timer->stop();
			d->osd->clear();
			d->cpu = d->vfps = -1;
		}
	}
}

void PlayInfoView::update() {
	static const QString br("<br>");
	static const QString colon(": ");
	if (d->visible) {
		app()->getProcInfo();
		QString text;
		text += tr("CPU Usage"); text += colon; text += d->cpu; text += br;
		text += tr("Memory Usage"); text += colon; text += d->mem; text += br;
		text += br;
		text += tr("Video Information"); text += br;
		text += tr("Pixel Size"); text += colon; text += d->size; text += br;
		text += tr("Input"); text += colon; text += d->vinput; text += br;
		text += tr("Output"); text += colon;
		text += d->voutput.arg(toString(d->video->outputFrameRate(), 2)); text += br;
		text += br;
		text += tr("Audio Information"); text += br;
		text += tr("Normalizer"); text += colon;
		text += toString(d->audio->gain()*100.0, 1); text += QLatin1String("%<br>");
		d->osd->showText(RichString(text, text));
	}
}

void PlayInfoView::setAudio(const AudioController *audio) {
	d->audio = audio;
	connect(d->audio, SIGNAL(formatChanged(AudioFormat)), this, SLOT(setAudioFormat(AudioFormat)));
}

void PlayInfoView::setAudioFormat(const AudioFormat &afmt) {
	d->afmt = afmt;
}
