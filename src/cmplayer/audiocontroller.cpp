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
	static constexpr bool isInt = true;
};
template<> struct AcMisc<float> {
	static constexpr float MaxLv = 1.0;
	static constexpr float MinLv = -1.0;
	static constexpr int format = AF_FORMAT_FLOAT_NE;
	static constexpr int bps = 4;
	static constexpr bool isInt = false;
};

// some codes are copied from mplayer
af_info af_info_dummy = { "CMPlayer audio controller", "dummy", "xylosper", "", AF_FLAGS_NOT_REENTRANT, ac_open };

extern af_info af_info_scaletempo;

struct AudioController::Data {
	struct SampleInfo { float avg = 0.0; int len = 0; void reset() {avg = 0.0; len = 0;}};

	mp_audio data;
	af_instance *af = nullptr;
	af_instance af_scaletempo;
	// volume
	int enable[AF_NCH];
	float level[AF_NCH];
	bool soft = true;

	template<typename T>
	void updateGain(mp_audio *data) {
		T *p = static_cast<T*>(data->audio);
		const int len = data->len/sizeof(T);
		float avg = 0.0;
		for (int i=0; i<len; ++i)
			avg += p[i]*p[i];
		avg = qSqrt(avg/(float)len);

		float hint = avg;
		int total = len;
		for (const SampleInfo &sample : this->normalizedSamples) {
			hint += sample.avg*(float)sample.len;
			total += sample.len;
		}
		hint /= (float)total*(float)AcMisc<T>::MaxLv;
		if (hint >= 0.01)
			gain = clamp(0.25*(float)AcMisc<T>::MaxLv / hint, 0.1, 5.0);
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

	bool normalizer = false;
	bool scaletempo = false;
	QVector<SampleInfo> normalizedSamples;
	int index = 0;
	float gain = 1.0;

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


int AudioController::init(af_instance *af, const af_cfg *cfg) {
	Q_ASSERT(!af->setup);
	AudioController *ac = nullptr;
	for (auto list = cfg->list; *list; ++list) {
		auto tokens = QString::fromLatin1(*cfg->list).split('=');
		if (tokens.size() == 2 && tokens[0] == "dummy") {
			ac = (AudioController*)(void*)(quintptr)tokens[1].toULongLong();
			break;
		}
	}
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

	d->data.rate = d->af_scaletempo.data->rate;
	d->data.nch = d->af_scaletempo.data->nch;
	d->data.format = d->af_scaletempo.data->format;
	d->data.bps = d->af_scaletempo.data->bps;
	return af_test_output(d->af, data);
}

int AudioController::control(af_instance *af, int cmd, void *arg) {
	auto ac = static_cast<AudioController*>(af->setup);
	auto d = ac ? ac->d : nullptr;
	Q_ASSERT(!ac || (ac && d));
	switch(cmd){
	case AF_CONTROL_REINIT:
		return ac->config(static_cast<mp_audio*>(arg));
	case AF_CONTROL_POST_CREATE:
		return ac->init(af, static_cast<af_cfg*>(arg));
	case AF_CONTROL_COMMAND_LINE:
		return AF_OK;
	case AF_CONTROL_VOLUME_ON_OFF | AF_CONTROL_SET:
		memcpy(d->enable, (int*)arg, AF_NCH*sizeof(int));
		return AF_OK;
	case AF_CONTROL_VOLUME_ON_OFF | AF_CONTROL_GET:
		memcpy((int*)arg, d->enable, AF_NCH*sizeof(int));
		return AF_OK;
	case AF_CONTROL_VOLUME_SOFTCLIP | AF_CONTROL_SET:
		d->soft = *(int*)arg;
		return AF_OK;
	case AF_CONTROL_VOLUME_SOFTCLIP | AF_CONTROL_GET:
		*(int*)arg = d->soft;
		return AF_OK;
	case AF_CONTROL_VOLUME_LEVEL | AF_CONTROL_SET:
		return af_from_dB(AF_NCH, (float*)arg, d->level, 20.0, -200.0, 60.0);
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
