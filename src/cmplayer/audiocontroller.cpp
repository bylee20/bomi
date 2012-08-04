#include "audiocontroller.hpp"
#include <QtCore/QStringList>
#include <QtCore/QMetaType>
#include <QtCore/QVector>
#include "avmisc.hpp"
#include <cmath>
#include <QtCore/QDebug>
#include "playengine.hpp"
#include "mpcore.hpp"

extern "C" {
#include <mixer.h>
#include <libmpdemux/demuxer.h>
#include <libmpdemux/stheader.h>
static void checkvolume(struct mixer *mixer)
{
	if (!mixer->ao)
		return;

	ao_control_vol_t vol;
	if (mixer->softvol || CONTROL_OK != ao_control(mixer->ao, AOCONTROL_GET_VOLUME, &vol)) {
		mixer->softvol = true;
		if (!mixer->afilter)
			return;
		float db_vals[AF_NCH];
		if (!af_control_any_rev(mixer->afilter,
						AF_CONTROL_VOLUME_LEVEL | AF_CONTROL_GET, db_vals))
			db_vals[0] = db_vals[1] = 1.0;
		else
			af_from_dB(2, db_vals, db_vals, 20.0, -200.0, 60.0);
		vol.left = (db_vals[0] / (mixer->softvol_max / 100.0)) * 100.0;
		vol.right = (db_vals[1] / (mixer->softvol_max / 100.0)) * 100.0;
	}
	float l = mixer->vol_l;
	float r = mixer->vol_r;
	if (mixer->muted_using_volume)
		l = r = 0;
	/* Try to detect cases where the volume has been changed by some external
	 * action (such as something else changing a shared system-wide volume).
	 * We don't test for exact equality, as some AOs may round the value
	 * we last set to some nearby supported value. 3 has been the default
	 * volume step for increase/decrease keys, and is apparently big enough
	 * to step to the next possible value in most setups.
	 */
	if (FFABS(vol.left - l) >= 3 || FFABS(vol.right - r) >= 3) {
		mixer->vol_l = vol.left;
		mixer->vol_r = vol.right;
		if (mixer->muted_using_volume)
			mixer->muted = false;
	}
	if (!mixer->softvol)
		// Rely on the value not changing if the query is not supported
		ao_control(mixer->ao, AOCONTROL_GET_MUTE, &mixer->muted);
	mixer->muted_by_us &= mixer->muted;
	mixer->muted_using_volume &= mixer->muted;
}

static void setvolume_internal(mixer_t *mixer, float l, float r)
{
	struct ao_control_vol vol = {.left = l, .right = r};
	if (!mixer->softvol) {
		// relies on the driver data being permanent (so ptr stays valid)
		mixer->restore_volume = mixer->ao->no_persistent_volume ?
			mixer->ao->driver->info->short_name : NULL;
		if (ao_control(mixer->ao, AOCONTROL_SET_VOLUME, &vol) != CONTROL_OK)
			mp_tmsg(MSGT_GLOBAL, MSGL_ERR,
					"[Mixer] Failed to change audio output volume.\n");
		return;
	}
	mixer->restore_volume = "softvol";
	if (!mixer->afilter)
		return;
	// af_volume uses values in dB
	float db_vals[AF_NCH];
	int i;
	db_vals[0] = (l / 100.0) * (mixer->softvol_max / 100.0);
	db_vals[1] = (r / 100.0) * (mixer->softvol_max / 100.0);
	for (i = 2; i < AF_NCH; i++)
		db_vals[i] = ((l + r) / 100.0) * (mixer->softvol_max / 100.0) / 2.0;
	af_to_dB(AF_NCH, db_vals, db_vals, 20.0);
	if (!af_control_any_rev(mixer->afilter,
							AF_CONTROL_VOLUME_LEVEL | AF_CONTROL_SET,
							db_vals)) {
		mp_tmsg(MSGT_GLOBAL, MSGL_INFO,
				"[Mixer] No hardware mixing, inserting volume filter.\n");
		if (!(af_add(mixer->afilter, (char*)"volume")
			  && af_control_any_rev(mixer->afilter,
									AF_CONTROL_VOLUME_LEVEL | AF_CONTROL_SET,
									db_vals)))
			mp_tmsg(MSGT_GLOBAL, MSGL_ERR,
					"[Mixer] No volume control available.\n");
	}
}

void mixer_setvolume2(mixer_t *mixer, float l, float r)
{
	checkvolume(mixer);  // to check mute status and AO support for volume
	mixer->vol_l = qBound(0.0f, l, 100.0f);//av_clip(l, 0, 100);
	mixer->vol_r = qBound(0.0f, r, 100.0f);//av_clip(r, 0, 100);
	if (!mixer->ao || mixer->muted)
		return;
	setvolume_internal(mixer, mixer->vol_l, mixer->vol_r);
}

}

struct AudioController::Data {
	PlayEngine *engine = nullptr;
	MPContext *mpctx = nullptr;
	StreamList streams;
	int volume = 100;
	double amp = 1.0;
	bool muted = false, volnorm = false, scaletempo = false;
	inline bool isRunning() const {return engine && (engine->isPlaying() || engine->isPaused());}
	inline void af(const char *name, bool on) {if (isRunning()) engine->tellmp(on ? "af_add" : "af_del", name);}
};

void plug(PlayEngine *engine, AudioController *audio) {
	audio->d->engine = engine;
	audio->d->mpctx = engine->context();
	engine->m_audio = audio;
	QObject::connect(engine, SIGNAL(aboutToPlay()), audio, SLOT(onAboutToPlay()), Qt::DirectConnection);
}

void unplug(PlayEngine *engine, AudioController *audio) {
	Q_ASSERT(audio->d->engine == engine);
	audio->d->engine = nullptr;
	engine->m_audio = nullptr;
	QObject::disconnect(engine, SIGNAL(aboutToPlay()), audio, SLOT(onAboutToPlay()));
}


AudioController::AudioController()
: d(new Data) {
}

AudioController::~AudioController() {
	delete d;
}

StreamList AudioController::streams() const {
	return d->streams;
}

void AudioController::onAboutToOpen() {
	d->streams.clear();
}

void AudioController::onAboutToPlay() {
	if (d->engine && d->engine->context()) {
		MPContext *mpctx = d->engine->context();
		auto mixer = &d->engine->context()->mixer;
//		mixer_setvolume2(mixer, 0.0f, 0.0f); mplayer bug? when volume < 1.0, no sound output
		mixer_setmute(mixer, d->muted);
		QStringList afs;
		if (d->volnorm)
			afs << "volnorm";
		if (d->scaletempo)
			afs << "scaletempo";
		if (!afs.isEmpty())
			d->engine->tellmp("af_add", afs);
	}
}

int AudioController::volume() const {
	return d->volume;
}

bool AudioController::isMuted() const {
	return d->muted;
}

double AudioController::realVolume() const {
	return qBound(0.0, d->amp * d->volume, 1000.0)/10.0;
}

void AudioController::setVolume(int volume) {
	volume = qBound(0, volume, 100);
	if (d->volume != volume) {
		d->volume = volume;
		if (d->isRunning())
			d->engine->enqueue(new PlayEngine::Cmd(PlayEngine::Cmd::Volume, realVolume()));
		emit volumeChanged(d->volume);
	}
}

void AudioController::setMuted(bool muted) {
	if (d->muted != muted) {
		d->muted = muted;
		if (d->isRunning())
			d->engine->tellmp("mute", (int)d->muted);
		emit mutedChanged(d->muted);
	}
}

void AudioController::setPreAmp(double amp) {
	amp = qFuzzyCompare(amp, 1.0) ? 1.0 : qBound(0.0, amp, 10.0);
	if (!qFuzzyCompare(d->amp, amp)) {
		d->amp = amp;
		if (d->isRunning())
			d->engine->enqueue(new PlayEngine::Cmd(PlayEngine::Cmd::Volume, realVolume()));
	}
}

double AudioController::preAmp() const {
	return d->amp;
}

void AudioController::setVolumeNormalized(bool norm) {
	if (d->volnorm != norm) {
		d->volnorm = norm;
		d->af("volnorm", d->volnorm);
		emit volumeNormalizedChanged(d->volnorm);
	}
}

bool AudioController::isVolumeNormalized() const {
	return d->volnorm;
}

void AudioController::setTempoScaled(bool scaled) {
	if (d->scaletempo != scaled) {
		d->scaletempo = scaled;
		d->af("scaletempo", d->scaletempo);
		emit tempoScaledChanged(d->scaletempo);
	}
}

bool AudioController::isTempoScaled() const {
	return d->scaletempo;
}

void AudioController::setCurrentStream(int id) const {
	if (d->engine)
		d->engine->tellmp("switch_audio", id);
}

int AudioController::currentStreamId() const {
	if (d->mpctx && d->mpctx->sh_audio)
		return d->mpctx->sh_audio->aid;
	return -1;
}

bool AudioController::parse(const Id &id) {
	if (getStream(id, "AUDIO", "AID", d->streams))
		return true;
	return false;
}

mixer *AudioController::mixer() const {
	return d->mpctx ? &d->mpctx->mixer : nullptr;
}
