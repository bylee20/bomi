#include "audiocontroller.hpp"
#include "audiofilter.hpp"
#include "channelmanipulation.hpp"
#include "enums.hpp"

af_info create_info();
af_info af_info_dummy = create_info();
struct cmplayer_af_priv { AudioController *ac; char *address; };
static AudioController *priv(af_instance *af) { return static_cast<cmplayer_af_priv*>(af->priv)->ac; }

struct AudioController::Data {
	struct BufferInfo {
		BufferInfo(int samples = 0): samples(samples) {}
		void reset() {level = 0.0; samples = 0;}
		double level = 0.0; int samples = 0;
	};

	bool normalizerActivated = false;
	bool tempoScalerActivated = false;
	bool volumeChanged = false;
	bool muted = false;
	double scale = 1.0;
	TempoScaler *scaler = nullptr;
	VolumeController *volume = nullptr;

	mp_audio data;
	af_instance *af = nullptr;
	float level[AF_NCH];

	NormalizerOption normalizerOption;

	ClippingMethod clip = ClippingMethod::Auto;
	ChannelLayoutMap map = ChannelLayoutMap::default_();
	ChannelLayout layout = ChannelLayout::Default;
};

AudioController::AudioController(QObject *parent): QObject(parent), d(new Data) {
	std::fill_n(d->level, AF_NCH, 1.0);
	memset(&d->data, 0, sizeof(d->data));
}

AudioController::~AudioController() {
	delete d->scaler;
	delete d->volume;
	delete d;
}

void AudioController::setClippingMethod(ClippingMethod method) {
	d->clip = method;
}

int AudioController::open(af_instance *af) {
	auto priv = static_cast<cmplayer_af_priv*>(af->priv);
	priv->ac = address_cast<AudioController*>(priv->address);
	auto d = priv->ac->d;
	d->af = af;

	af->control = AudioController::control;
	af->uninit = AudioController::uninit;
	af->filter = AudioController::filter;
	af->mul = 1;
	af->data = &d->data;


	return AF_OK;
}

void AudioController::uninit(af_instance *af) {
	memset(af->data, 0, sizeof(d->data));
	auto ac = priv(af);
	if (ac)
		ac->d->af = nullptr;
}

template<typename Filter>
Filter *check(Filter *&filter, const mp_audio *data, const mp_audio *input) {
	if (!filter || !filter->isCompatibleWith(data, input)) {
		delete filter;
		filter = Filter::create(data->format);
	}
	if (filter)
		filter->reconfigure(data, input);
	return filter;
}

VolumeController *check(VolumeController *&filter, ClippingMethod clip, const mp_audio *data, const mp_audio *input) {
	if (!filter || !filter->isCompatibleWith(data, input) || filter->clippingMethod() != clip) {
		delete filter;
		filter = VolumeController::create(data->format, clip);
	}
	if (filter)
		filter->reconfigure(data, input);
	return filter;
}

void AudioController::setChannelLayout(ChannelLayout layout) {
	d->layout = layout;
}

int AudioController::reinitialize(mp_audio *data) {
	d->volumeChanged = false;
	if (!data)
		return AF_ERROR;
	mp_audio_force_interleaved_format(data);
	mp_audio_copy_config(&d->data, data);
	switch (data->format) {
	case AF_FORMAT_S16:
	case AF_FORMAT_S32:
	case AF_FORMAT_FLOAT:
	case AF_FORMAT_DOUBLE:
		break;
	default:
		mp_audio_set_format(&d->data, AF_FORMAT_FLOAT);
	}
	if (d->layout != ChannelLayout::Default) {
		mp_chmap map;
		if (mp_chmap_from_str(&map, bstr0(ChannelLayoutInfo::data(d->layout).constData())))
			mp_audio_set_channels(&d->data, &map);
		else
			qDebug() << "Cannot load channel layout:" << ChannelLayoutInfo::name(d->layout);
	}
	if (!af_test_output(d->af, data))
		return false;
	check(d->volume, d->clip, &d->data, data);
	check(d->scaler, &d->data, &d->data);
	d->volume->setChannelLayoutMap(d->layout == ChannelLayout::Default ? ChannelLayoutMap() : d->map);
	return true;
}

void AudioController::setLevel(double level) {
	std::fill_n(d->level, AF_NCH, level);
}

double AudioController::level() const {
	return d->level[0];
}

void AudioController::setMuted(bool muted) {
	d->muted = muted;
}

bool AudioController::isMuted() const {
	return d->muted;
}

int AudioController::control(af_instance *af, int cmd, void *arg) {
	auto ac = priv(af);
	auto d = ac->d;
	switch(cmd){
	case AF_CONTROL_REINIT:
		return ac->reinitialize(static_cast<mp_audio*>(arg));
	case AF_CONTROL_SET_VOLUME:
		return AF_OK;
	case AF_CONTROL_GET_VOLUME:
		*((float*)arg) = d->level[0];
		return AF_OK;
	case AF_CONTROL_SET_PLAYBACK_SPEED:
		d->scale = *(double*)arg;
	default:
		return AF_UNKNOWN;
	}
}

int AudioController::filter(af_instance *af, mp_audio *data, int /*flags*/) {
	auto ac = priv(af); auto d = ac->d;
	af->mul = 1.0;
	af->delay = 0.0;
	Q_ASSERT(d->volume != nullptr);
	if (d->volume && d->volume->prepare(ac, &d->data, data)) {
		data = d->volume->play(data);
		af->mul *= d->volume->multiplier();
	}
	if (d->scaler && d->scaler->prepare(ac, data, data)) {
		data = d->scaler->play(data);
		af->mul *= d->scaler->multiplier();
		af->delay = d->scaler->delay();
	}
	return 0;
}

bool AudioController::setNormalizerActivated(bool on) {
	return _Change(d->normalizerActivated, on);
}

bool AudioController::setTempoScalerActivated(bool on) {
	return _Change(d->tempoScalerActivated, on);
}

double AudioController::gain() const {
	return d->normalizerActivated && d->volume ? d->volume->gain() : 1.0;
}

bool AudioController::isTempoScalerActivated() const {
	return d->tempoScalerActivated;
}

double AudioController::scale() const {
	return d->scale;
}

void AudioController::setNormalizerOption(double length, double target, double silence, double min, double max) {
	d->normalizerOption.bufferLengthInSeconds = length;
	d->normalizerOption.targetLevel = target;
	d->normalizerOption.silenceLevel = silence;
	d->normalizerOption.minimumGain = min;
	d->normalizerOption.maximumGain = max;
}

bool AudioController::isNormalizerActivated() const {
	return d->normalizerActivated;
}

const NormalizerOption &AudioController::normalizerOption() const {
	return d->normalizerOption;
}

void AudioController::setChannelLayoutMap(const ChannelLayoutMap &map) {
	d->map = map;
}

af_info create_info() {
	static m_option options[2];
	memset(options, 0, sizeof(options));
	options[0].name = "address";
	options[0].flags = 0;
	options[0].defval = 0;
	options[0].offset = MP_CHECKED_OFFSETOF(cmplayer_af_priv, address, char*);
	options[0].is_new_option = 1;
	options[0].type = &m_option_type_string;

	static af_info info = {
		"CMPlayer audio controller",
		"dummy",
		AF_FLAGS_NOT_REENTRANT,
		AudioController::open,
		nullptr,
		sizeof(cmplayer_af_priv),
		nullptr,
		options
	};
	return info;
}
