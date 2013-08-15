#include "audiocontroller.hpp"
#include <mpvcore/mp_cmplayer.h>
extern "C" {
#include <audio/filter/af.h>
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

struct cmplayer_af_priv { AudioController *ac; char *address; };

static AudioController *priv(af_instance *af) { return static_cast<cmplayer_af_priv*>(af->priv)->ac; }

#define OPT_BASE_STRUCT struct cmplayer_af_priv
af_info create_info() {
	static m_option options[2];
	memset(options, 0, sizeof(options));
	options[0].name = "address";
	options[0].flags = 0;
	options[0].defval = 0;
	options[0].offset = MP_CHECKED_OFFSETOF(OPT_BASE_STRUCT, address, char*);
	options[0].____new = 1;
	options[0].type = &m_option_type_string;

	static af_info info = {
		"CMPlayer audio controller",
		"dummy",
		"xylosper",
		"",
		AF_FLAGS_NOT_REENTRANT,
		AudioController::open,
		nullptr,
		sizeof(cmplayer_af_priv),
		nullptr,
		options
	};
	return info;
}

af_info af_info_dummy = create_info();

extern af_info af_info_scaletempo;

struct AudioController::Data {
	struct BufferInfo {
		BufferInfo(int samples = 0): samples(samples) {}
		void reset() {level = 0.0; samples = 0;}
		double level = 0.0; int samples = 0;
	};
	mp_audio data;
	double seconds = 10.0;
	bool muted = false;
	af_instance *af = nullptr, af_scaletempo;
	int enable[AF_NCH];
	float level[AF_NCH];
	double gain = 1.0, silence = 0.0001, gain_min = 0.1, gain_max = 10.0, target = 0.25;
	QLinkedList<BufferInfo> buffers;
	QLinkedList<BufferInfo>::iterator it;

	template<typename T>
	void updateGain(mp_audio *data) {
		T *p = static_cast<T*>(data->audio);
		const int size = data->len/sizeof(T);

		BufferInfo buffer(size/data->channels.num);
		for (int i=0; i<size; ++i)
			buffer.level += AcMisc<T>::level(p[i]);
		buffer.level /= (double)size;

		BufferInfo total;
		for (const auto &info : this->buffers) {
			total.level += info.level*info.samples;
			total.samples += info.samples;
		}
		total.level += buffer.level*buffer.samples;
		total.samples += buffer.samples;
		const double average = total.level/total.samples;
		const double go = (average > silence) ? qBound(gain_min, target / average, gain_max) : 1.0;
		const double rate = go/gain;
		if (rate > 1.05) {
			gain *= 1.05;
		} else if (rate < 0.95)
			gain *= 0.95;
		else
			gain = go;
		if ((double)total.samples/(double)data->rate >= seconds) {
			if (++it == buffers.end())
				it = buffers.begin();
			*it = buffer;
		} else {
			buffers.push_back(buffer);
			it = --buffers.end();
		}
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
};

AudioController::AudioController(QObject *parent): QObject(parent), d(new Data) {
	for (int i=0; i<AF_NCH; ++i) {
		d->enable[i] = 1;
		d->level[i] = 1.0;
	}
	d->af_scaletempo.priv = malloc(af_info_scaletempo.priv_size);
	memcpy(d->af_scaletempo.priv, af_info_scaletempo.priv_defaults, af_info_scaletempo.priv_size);
}

AudioController::~AudioController() {
	free(d->af_scaletempo.priv);
	delete d;
}

int AudioController::open(af_instance *af) {
	af->control = AudioController::control;
	af->uninit = AudioController::uninit;
	af->play = AudioController::play;
	af->mul = 1;
	af->data = nullptr;
	af->setup = nullptr;

	auto priv = static_cast<cmplayer_af_priv*>(af->priv);
	priv->ac = (AudioController*)(void*)(quintptr)QString::fromLatin1(priv->address).toULongLong();

	auto d = priv->ac->d;
	af->data = &d->data;

	d->af = af;
	af_info_scaletempo.open(&d->af_scaletempo);
	d->af_scaletempo.info = &af_info_scaletempo;

	return AF_OK;
}

void AudioController::uninit(af_instance *af) {
	auto ac = priv(af);
	if (ac) {
		ac->d->af_scaletempo.uninit(&ac->d->af_scaletempo);
		ac->d->af = nullptr;
	}
}

int AudioController::config(mp_audio *data) {
	if (!data || d->af_scaletempo.control(&d->af_scaletempo, AF_CONTROL_REINIT, data) <= 0)
		return AF_ERROR;
	d->gain = 1.0;
	d->buffers.clear();

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
	auto ac = priv(af);
	auto d = ac->d;
	switch(cmd){
	case AF_CONTROL_REINIT:
		return ac->config(static_cast<mp_audio*>(arg));
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
	auto d = priv(af)->d;
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
		d->buffers.clear();
		d->gain = 1.0;
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

void AudioController::setNormalizer(double length, double target, double silence, double min, double max) {
	d->seconds = length;
	d->silence = silence;
	d->target = target;
	d->gain_min = min;
	d->gain_max = max;
}
