#include "audiocontroller.hpp"
extern "C" {
#define new __new
#include <audio/filter/af.h>
#undef new
}

int ac_open(af_instance *af) {
	af->control = AudioController::control;
	af->uninit = AudioController::uninit;
	af->play = AudioController::play;
	af->mul = 1;
	af->data = nullptr;
	af->setup = nullptr;
	return AF_OK;
}

template<typename T> struct AcMisc {};
template<> struct AcMisc<qint16> {
	static constexpr int MaxLv = SHRT_MAX;
	static constexpr int MinLv = SHRT_MIN;
	static constexpr int format = AF_FORMAT_S16_NE;
	static constexpr int bps = 2;
	static constexpr double level(double p) {return (p < 0 ? -p : p)/(double)(SHRT_MAX);}
	static constexpr bool isInt = true;
};
template<> struct AcMisc<float> {
	static constexpr float MaxLv = 1.0;
	static constexpr float MinLv = -1.0;
	static constexpr int format = AF_FORMAT_FLOAT_NE;
	static constexpr int bps = 4;
	static constexpr bool isInt = false;
	static constexpr double level(double p) {return (p < 0 ? -p : p);}
};

// some codes are copied from mplayer
af_info af_info_dummy = { "CMPlayer audio controller", "dummy", "xylosper", "", AF_FLAGS_NOT_REENTRANT, ac_open, nullptr};

extern af_info af_info_scaletempo;

struct AudioController::Data {
	struct SampleInfo { double avg = 0.0; int len = 0; void reset() {avg = 0.0; len = 0;}};
	mp_audio data;
	bool muted = false;
	af_instance *af = nullptr, af_scaletempo;
	int enable[AF_NCH], index = 0; float level[AF_NCH];
	double gain = 1.0, silence = 0.0001, gain_min = 0.1, gain_max = 10.0, target = 0.25;
	template<typename T>
	void updateGain(mp_audio *data) {
		T *p = static_cast<T*>(data->audio);
		const int len = data->len/sizeof(T);
		double avg = 0.0;
		for (int i=0; i<len; ++i) {
			avg += AcMisc<T>::level(p[i]);
		}
		avg /= (double)len;

		double hint = 0.0; int total = 0;
		for (const SampleInfo &sample : this->normalizedSamples) {
			hint += sample.avg*(double)sample.len;
			total += sample.len;
		}
		hint /= (double)total;
		if (hint >= silence)
			gain = qBound(gain_min, target / hint, gain_max);
		normalizedSamples[index].len = len;
		normalizedSamples[index].avg = avg*gain;
		index = (index+1)%normalizedSamples.size();
	}
	template<typename T, typename Clamp>
	mp_audio *playVolume(mp_audio *data, const Clamp &fclamp) {
		T *p = static_cast<T*>(data->audio);
		const int len = data->len/sizeof(T), nch = data->nch;
		for (int ch = 0; ch < nch; ++ch) {
			if (!enable[ch])
				continue;
			for (int i = ch; i < len; i += nch)
				p[i] = fclamp(p[i]*level[ch]*gain);
		}
		return data;
	}
	bool soft = true, normalizer = false, scaletempo = false;
	QVector<SampleInfo> normalizedSamples;
};

AudioController::AudioController(QObject *parent): QObject(parent), d(new Data) {
	for (int i=0; i<AF_NCH; ++i) {
		d->enable[i] = 1;
		d->level[i] = 1.0;
	}
}

AudioController::~AudioController() {
	delete d;
}


int AudioController::init(af_instance *af, const char *arg) {
	Q_ASSERT(!af->setup);
	AudioController *ac = (AudioController*)(void*)(quintptr)QString::fromLatin1(arg).toULongLong();
	Q_ASSERT(ac);
	auto d = ac->d;
	af->setup = ac;
	af->data = &d->data;

	d->af = af;
	af_info_scaletempo.open(&d->af_scaletempo);
	d->af_scaletempo.info = &af_info_scaletempo;
	return AF_OK;
}

void AudioController::uninit(af_instance *af) {
	auto ac = static_cast<AudioController*>(af->setup);
	if (ac) {
		ac->d->af_scaletempo.uninit(&ac->d->af_scaletempo);
		ac->d->af = nullptr;
	}
}

int AudioController::config(mp_audio *data) {
	if (!data || d->af_scaletempo.control(&d->af_scaletempo, AF_CONTROL_REINIT, data) <= 0)
		return AF_ERROR;
	d->index = 0;
	d->gain = 1.0;
	d->normalizedSamples.clear();
	d->normalizedSamples.resize(128);

	d->af->delay = d->af_scaletempo.delay;
	d->af->mul = d->af_scaletempo.mul;

	mp_audio_copy_config(&d->data, d->af_scaletempo.data);
	return af_test_output(d->af, data);
}

void AudioController::setVolume(double volume) {
	for (int i=0; i<AF_NCH; ++i)
		d->level[i] = volume;
}

double AudioController::volume() const {
	return d->level[0];
}

int AudioController::control(af_instance *af, int cmd, void *arg) {
	auto ac = static_cast<AudioController*>(af->setup);
	auto d = ac ? ac->d : nullptr;
	Q_ASSERT(!ac || (ac && d));
	switch(cmd){
	case AF_CONTROL_REINIT:
		Q_ASSERT(ac != nullptr);
		return ac->config(static_cast<mp_audio*>(arg));
	case AF_CONTROL_COMMAND_LINE:
		init(af, static_cast<const char*>(arg));
		return AF_OK;
	case AF_CONTROL_VOLUME_LEVEL | AF_CONTROL_SET:
//		return af_from_dB(AF_NCH, (float*)arg, d->level, 20.0, -200.0, 60.0);
		return AF_OK;
	case AF_CONTROL_VOLUME_LEVEL | AF_CONTROL_GET:
		return af_to_dB(AF_NCH, d->level, (float*)arg, 20.0);
	case AF_CONTROL_PLAYBACK_SPEED | AF_CONTROL_SET:
	case AF_CONTROL_SCALETEMPO_AMOUNT | AF_CONTROL_SET:
	case AF_CONTROL_SCALETEMPO_AMOUNT | AF_CONTROL_GET:
		return d->scaletempo ? d->af_scaletempo.control(&d->af_scaletempo, cmd, arg) : AF_UNKNOWN;
	default:
		return AF_UNKNOWN;
	}
}

mp_audio *AudioController::play(af_instance *af, mp_audio *data) {
	auto d = static_cast<AudioController*>(af->setup)->d;
	if (d->normalizer) {
		if (d->data.format == AF_FORMAT_S16_NE)
			d->updateGain<qint16>(data);
		else if (d->data.format == AF_FORMAT_FLOAT_NE)
			d->updateGain<float>(data);
	} else
		d->gain = 1.0;
	if (d->scaletempo) {
		data = d->af_scaletempo.play(&d->af_scaletempo, data);
		af->delay = d->af_scaletempo.delay;
	}
	if (d->data.format == AF_FORMAT_S16_NE)
		return d->playVolume<qint16>(data, [](int v) {return clamp(v, SHRT_MIN, SHRT_MAX);});
	else if (d->data.format == AF_FORMAT_FLOAT_NE) {
		if (d->soft)
			return d->playVolume<float>(data, [](float v) {return af_softclip(v);});
		else
			return d->playVolume<float>(data, [](float v) {return clamp(v, -1.0, 1.0);});
	}
	return data;
}

bool AudioController::setNormalizer(bool on) {
	if (_Change(d->normalizer, on)) {
		for (auto &sample : d->normalizedSamples)
			sample.reset();
		return true;
	} else
		return false;
}

bool AudioController::setScaletempo(bool on) {
	return _Change(d->scaletempo, on);
}

double AudioController::normalizer() const {
	return d->normalizer ? d->gain : -1.0;
}

bool AudioController::scaletempo() const {
	return d->scaletempo;
}

void AudioController::setNormalizer(double target, double silence, double min, double max) {
	d->silence = silence;
	d->target = target;
	d->gain_min = min;
	d->gain_max = max;
}
