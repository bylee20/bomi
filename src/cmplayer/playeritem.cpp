#include "playeritem.hpp"
#include "playengine.hpp"
#include "videorendereritem.hpp"
#include "playinfoitem.hpp"
#include "globalqmlobject.hpp"
#include "videoformat.hpp"

extern "C" {
#include <mpvcore/mp_core.h>
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
: QQuickItem(parent), d(new Data)
, m_audio(new AvInfoObject(this)), m_video(new AvInfoObject(this))
, m_media(new MediaInfoObject(this)) {
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
	Skin::unplug();
	if (m_renderer)
		m_renderer->setParentItem(nullptr);
	m_engine = nullptr;
	m_renderer = nullptr;
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
			setDuration(m_engine->duration());
		}
	});
	plug(m_engine, &PlayEngine::stateChanged, [this] (EngineState state) { setState((State)state); });
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
	plug(m_engine, &PlayEngine::speedChanged, [this] (double speed) {
		emit speedChanged(m_speed = speed);
	});
	if (m_renderer) {
		d->drawnFrames = m_renderer->drawnFrames();
		d->frameTime = UtilObject::systemTime();
	}

	setState((State)m_engine->state());
	emit speedChanged(m_speed = m_engine->speed());
	emit mutedChanged(m_muted = m_engine->isMuted());
	emit durationChanged(m_duration = m_engine->duration());
	emit tick(m_position = m_engine->position());
	emit mediaChanged();
	emit volumeChanged(m_volume = m_engine->volume());

	m_video->setVideo(m_engine);
	m_audio->setAudio(m_engine);
	emit videoChanged();
	emit audioChanged();
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
	if (m_renderer)
		d->sync -= m_renderer->delay();
	return sync;
}

double PlayerItem::volumeNormalizer() const {
	return m_engine ? m_engine->volumeNormalizer() : 1.0;
}

bool PlayerItem::isVolumeNormalizerActivated() const {
	return m_engine ? m_engine->isVolumeNormalized() : false;
}

QString PlayerItem::stateText() const {
	switch (m_state) {
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


