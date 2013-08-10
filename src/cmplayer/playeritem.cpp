#include "playeritem.hpp"
#include "playengine.hpp"
#include "videorendereritem.hpp"
#include "playinfoitem.hpp"
#include "rootmenu.hpp"
#include "globalqmlobject.hpp"
#include "playinfoitem.hpp"
#include "playlistmodel.hpp"
#include "videorendereritem.hpp"
#include "playengine.hpp"
#include <mpvcore/mp_cmplayer.h>
#include "videoformat.hpp"

extern "C" {
#include <demux/stheader.h>
#include <mpvcore/codecs.h>
#include <video/decode//vd.h>
#include <video/out/vo.h>
#include <audio/out/ao.h>
#include <audio/filter/af.h>
}

static QObject *utilProvider(QQmlEngine *, QJSEngine *) {return new UtilObject;}
static QObject *settingsProvider(QQmlEngine *, QJSEngine *) {return new SettingsObject;}

struct PlayerItem::Data {
	quint64 frameTime = 0, drawnFrames = 0;
	double sync = 0.0;
	QTimer timer;
	int time = -1;
};

PlayerItem::PlayerItem(QQuickItem *parent)
: QQuickItem(parent), d(new Data) {
}

PlayerItem::~PlayerItem() {
	delete d;
}

void PlayerItem::setState(State state) {
	if (_Change(m_state, state)) {
		emit stateChanged(m_state);
		bool running = false, playing = false;
		if (m_engine) {
			emit stateTextChanged(stateText());
			const auto state = m_engine->state();
			running = state != EngineStopped && state != EngineFinished && state != EngineError;
			playing = running && state != EnginePaused;
		}
		if (_Change(m_running, running))
			emit runningChanged(m_running);
		if (_Change(m_playing, playing))
			emit playingChanged(m_playing);
	}
}

void PlayerItem::unplug() {
	if (m_engine) {
		Skin::unplug();
		if (m_renderer)
			m_renderer->setParentItem(nullptr);
	}
}

void PlayerItem::plugTo(PlayEngine *engine) {
	if (m_engine == engine)
		return;
	m_engine = engine;
	m_renderer = engine->videoRenderer();
	m_renderer->setParentItem(this);
	m_renderer->setGeometry(QPointF(0, 0), QSizeF(width(), height()));

	auto mediaName = [this] (const Mrl &mrl) -> QString {
		if (mrl.isLocalFile())
			return tr("File") % _L(": ") % QFileInfo(mrl.toLocalFile()).fileName();
		else if (mrl.isDvd()) {
			if (m_engine && !m_engine->dvd().volume.isEmpty())
				return _L("DVD: ") % m_engine->dvd().volume;
			return _L("DVD: ") % mrl.toLocalFile();
		} else
			return _L("URL: ") % mrl.toString();
	};

	m_media->m_mrl = m_engine->mrl();
	m_media->setName(mediaName(m_media->m_mrl));
	plug(m_engine, &PlayEngine::mrlChanged, [this, mediaName] (const Mrl &mrl) {
		m_media->m_mrl = mrl;
		m_media->setName(mediaName(mrl));
	});
	plug(m_engine, &PlayEngine::tick, [this] (int pos) {
		if (isVisible()) {
			setPosition(pos);
			setState((State)m_engine->state());
			setDuration(m_engine->duration());
		}
	});

	plug(m_engine, &PlayEngine::videoFormatChanged, [this] () {
		m_video->setVideo(m_engine);
		emit videoChanged();
	});
	plug(m_engine, &PlayEngine::started, [this, mediaName] () {
		m_media->setName(mediaName(m_media->m_mrl));
		m_audio->setAudio(m_engine);
		emit audioChanged();
		d->time = 0;
		d->sync = 0;
		emit mediaChanged();
	});

	plug(m_engine, &PlayEngine::volumeChanged, [this](int volume) {
		emit volumeChanged(m_volume = volume);
	});
	plug(m_engine, &PlayEngine::mutedChanged, [this] (bool muted) {
		emit mutedChanged(m_muted = muted);
	});

	if (m_renderer) {
		d->drawnFrames = m_renderer->drawnFrames();
		d->frameTime = UtilObject::systemTime();
	}

	setState((State)m_engine->state());
	emit mutedChanged(m_muted = m_engine->isMuted());
	emit durationChanged(m_duration = m_engine->duration());
	emit tick(m_position = m_engine->position());
	emit videoChanged();
	emit audioChanged();
	emit mediaChanged();
//	emit stateChanged(m_state = (State)m_engine->state());
	emit volumeChanged(m_volume = m_engine->volume());
}

void PlayerItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) {
	QQuickItem::geometryChanged(newGeometry, oldGeometry);
	if (m_renderer) {
		m_renderer->setPosition(QPointF(0, 0));
		m_renderer->setSize(QSizeF(width(), height()));
	}
}

void PlayerItem::registerItems() {
	qmlRegisterType<AvInfoObject>();
	qmlRegisterType<AvIoFormat>();
	qmlRegisterType<MediaInfoObject>();
	qmlRegisterType<PlayerItem>("CMPlayerCore", 1, 0, "Engine");
	qmlRegisterSingletonType<UtilObject>("CMPlayerCore", 1, 0, "Util", utilProvider);
	qmlRegisterSingletonType<SettingsObject>("CMPlayerCore", 1, 0, "Settings", settingsProvider);
}

void PlayerItem::updateStateInfo() {

}

void PlayerItem::seek(int time) {
	if (m_engine) {
		m_engine->seek(time);
	}
}

void PlayerItem::setVolume(int volume) {
	if (m_engine)
		m_engine->setVolume(volume);
}

void AvInfoObject::setVideo(const PlayEngine *engine) {
	auto mpctx = engine->context();
	if (!mpctx || !mpctx->sh_video || !mpctx->sh_video)
		return;
	const auto fmt = engine->videoFormat();
	auto sh = mpctx->sh_video;

	m_hwAcc = engine->isHwAccActivated();
	m_codec = _U(mpctx->sh[STREAM_VIDEO]->decoder_desc);
	m_input->m_type = format(sh->format);
	m_input->m_size = QSize(sh->disp_w, sh->disp_h);
	m_input->m_fps = sh->fps;
	m_input->m_bps = sh->i_bps*8;
	m_output->m_type = format(fmt.type());
	m_output->m_size = fmt.drawSize();
	m_output->m_fps = sh->fps;
	m_output->m_bps = fmt.bps(sh->fps);
}

void AvInfoObject::setAudio(const PlayEngine *engine) {
	auto mpctx = engine->context();
	if (!mpctx || !mpctx->sh_audio || !mpctx->ao)
		return;
	auto sh = mpctx->sh_audio;
	auto ao = mpctx->ao;
	m_hwAcc = false;
	m_codec = _U(mpctx->sh[STREAM_AUDIO]->decoder_desc);

	m_input->m_type = format(sh->format);
	m_input->m_bps = sh->i_bps*8;
	m_input->m_samplerate = sh->samplerate/1000.0; // kHz
	m_input->m_channels = sh->channels.num;
	m_input->m_bits = af_fmt2bits(sh->sample_format);

	m_output->m_type = _U(af_fmt2str_short(ao->format));
	m_output->m_bps = ao->bps*8;
	m_output->m_samplerate = ao->samplerate/1000.0;
	m_output->m_channels = ao->channels.num;
	m_output->m_bits = af_fmt2bits(ao->format);
}

double PlayerItem::avgfps() const {
	if (!m_renderer)
		return 0.0;
	const auto frameTime = UtilObject::systemTime();
	const auto drawnFrames = m_renderer->drawnFrames();
	double fps = 0.0;
	if (frameTime > d->frameTime && drawnFrames > d->drawnFrames)
		fps = double(drawnFrames - d->drawnFrames)/(double(frameTime - d->frameTime)*1e-6);
	d->drawnFrames = drawnFrames;
	d->frameTime = frameTime;
	return fps;
}

double PlayerItem::bps(double fps) const {
	return m_engine ? m_engine->videoFormat().bps(fps) : 0.0;
}

double PlayerItem::avgsync() const {
	if (!m_engine || !m_engine->context())
		return 0.0;
	auto mpctx = m_engine->context();
	double sync = 0.0;
	if (mpctx->sh_audio && mpctx->sh_video) {
		sync = (mpctx->last_av_difference)*1000.0;
		d->sync = mpctx->total_avsync_change;
	}
	return sync;
}

double PlayerItem::volumeNormalizer() const {
	return m_engine ? m_engine->volumeNormalizer() : 1.0;
}

QString PlayerItem::stateText() const {
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


