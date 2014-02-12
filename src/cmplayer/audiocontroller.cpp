#include "audiocontroller.hpp"
#include "audiomixer.hpp"
extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <audio/filter/af.h>
#include <audio/fmt-conversion.h>
}

af_info create_info();
af_info af_info_dummy = create_info();
struct cmplayer_af_priv { AudioController *ac; char *address; };
static AudioController *priv(af_instance *af) { return static_cast<cmplayer_af_priv*>(af->priv)->ac; }

static bool isSupported(int type) {
	switch (type) {
	case AF_FORMAT_S16P:
	case AF_FORMAT_S32P:
	case AF_FORMAT_FLOATP:
	case AF_FORMAT_DOUBLEP:
	case AF_FORMAT_S16:
	case AF_FORMAT_S32:
	case AF_FORMAT_FLOAT:
	case AF_FORMAT_DOUBLE:
		return true;
	default:
		return false;
	}
}

enum FilterDirty : quint32 {
	Normalizer = 1, Muted = 2, Amp = 4, ChMap = 8, Format = 16, Scale = 32, Resample = 64
};

struct AudioController::Data {
	quint32 dirty = 0;
	AudioMixer *mixer = nullptr;
	SwrContext *swr = nullptr;
	int fmt_conv = AF_FORMAT_UNKNOWN, outrate = 0;
	bool normalizerActivated = false, tempoScalerActivated = false, muted = false, resample = false, first = false;
	double scale = 1.0, amp = 1.0;
	mp_chmap chmap;
	mp_audio *resampled = nullptr;
	af_instance *af = nullptr;
//	QByteArray data;
	AudioNormalizerOption normalizerOption;
	ClippingMethod clip = ClippingMethod::Auto;
	ChannelLayoutMap map = ChannelLayoutMap::default_();
	ChannelLayout layout = ChannelLayout::Default;
};

AudioController::AudioController(QObject *parent): QObject(parent), d(new Data) {
}

AudioController::~AudioController() {
	delete d->mixer;
	delete d;
}

void AudioController::setClippingMethod(ClippingMethod method) {
	d->clip = method;
}

bool AudioController::test(int fmt_in, int fmt_out) {
	return isSupported(fmt_in) && isSupported(fmt_out);
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
	af->delay = 0.0;

	return AF_OK;
}

void AudioController::uninit(af_instance *af) {
	auto ac = priv(af); auto d = ac->d;
	Q_ASSERT(ac != nullptr);
	d->af = nullptr;
	if (d->swr)
		swr_free(&d->swr);
	talloc_free(d->resampled);
	d->resampled = nullptr;
}

AudioMixer *check(AudioMixer *&filter, ClippingMethod clip, const AudioFormat &in, const AudioFormat &out) {
	if (!filter || !filter->configure(in, out, clip)) {
		delete filter;
		filter = AudioMixer::create(in, out, clip);
	}
	Q_ASSERT(filter != nullptr);
	return filter;
}

int AudioController::reinitialize(mp_audio *in) {
	if (!in)
		return AF_ERROR;
	auto out = d->af->data;
	out->rate = in->rate;
	bool ret = true;
	if (!isSupported(in->format)) {
		ret = false;
		mp_audio_set_format(in, af_fmt_is_planar(in->format) ? AF_FORMAT_FLOATP : AF_FORMAT_FLOAT);
	}
	if (d->fmt_conv) {
		mp_audio_set_format(out, d->fmt_conv);
		d->fmt_conv = AF_FORMAT_UNKNOWN;
	} else
		mp_audio_set_format(out, in->format);
	d->chmap = in->channels;
	if (d->layout != ChannelLayout::Default && !mp_chmap_from_str(&d->chmap, bstr0(ChannelLayoutInfo::data(d->layout).constData())))
		qDebug() << "Cannot find matched channel layout for" << ChannelLayoutInfo::description(d->layout);
	mp_audio_set_channels(out, &d->chmap);
	if (d->outrate != 0)
		out->rate = d->outrate;
	if (!ret)
		return false;
	d->af->mul = (double)out->channels.num/in->channels.num;
	if (d->tempoScalerActivated)
		d->af->mul /= d->scale;
	if ((d->resample = out->rate != in->rate)) {
		d->af->mul *= (double)out->rate/in->rate;
		const auto nch = in->channels.num;/*mp_chmap_to_lavc_unchecked(&in->channels);*/
		const auto fmt = af_to_avformat(in->format);
		if (!d->swr)
			d->swr = swr_alloc();
		av_opt_set_int(d->swr,  "in_channel_count", nch, 0);
		av_opt_set_int(d->swr, "out_channel_count", nch, 0);
		av_opt_set_int(d->swr,  "in_sample_rate", in->rate, 0);
		av_opt_set_int(d->swr, "out_sample_rate", out->rate, 0);
		av_opt_set_sample_fmt(d->swr,  "in_sample_fmt", fmt, 0);
		av_opt_set_sample_fmt(d->swr, "out_sample_fmt", fmt, 0);
		swr_init(d->swr);
		if (!d->resampled)
			d->resampled = talloc_zero(nullptr, mp_audio);
		*d->resampled = *in;
		d->resampled->rate = out->rate;
		in = d->resampled;
	}
	const AudioFormat fmt_in(*in), fmt_out(*out);
	check(d->mixer, d->clip, fmt_in, fmt_out);
	d->mixer->setOutput(out);
	d->mixer->setChannelLayoutMap(d->map);
	d->dirty = 0xffffffff;
	d->first = true;
	return true;
}

void AudioController::setAmpLevel(double amp) {
	d->amp = amp;
	d->dirty |= Amp;
}

void AudioController::setMuted(bool muted) {
	d->muted = muted;
	d->dirty |= Muted;
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
		*((float*)arg) = d->amp;
		return AF_OK;
	case AF_CONTROL_SET_PLAYBACK_SPEED:
		d->scale = *(double*)arg;
		d->dirty |= Scale;
		return d->tempoScalerActivated;
	case AF_CONTROL_SET_FORMAT:
		d->fmt_conv = *(int*)arg;
		if (!isSupported(d->fmt_conv))
			d->fmt_conv = AF_FORMAT_UNKNOWN;
		return !!d->fmt_conv;
	case AF_CONTROL_SET_RESAMPLE_RATE:
		d->outrate = *(int *)arg;
		d->dirty |= Resample;
		return AF_OK;
	case AF_CONTROL_SET_CHANNELS:
		d->layout = ChannelLayoutMap::toLayout(*(mp_chmap*)arg);
		return d->layout != ChannelLayout::Default;
	case AF_CONTROL_RESET:
		if (d->swr)
			while (swr_drop_output(d->swr, 1000) > 0) ;
		return AF_OK;
	default:
		return AF_UNKNOWN;
	}
}

int AudioController::filter(af_instance *af, mp_audio *data, int /*flags*/) {
	auto ac = priv(af); auto d = ac->d;
	d->af->delay = 0.0;

	Q_ASSERT(d->mixer != nullptr);
	if (d->dirty) {
		if (d->dirty & Muted)
			d->mixer->setMuted(d->muted);
		if (d->dirty & Amp)
			d->mixer->setAmp(d->amp);
		if (d->dirty & Normalizer)
			d->mixer->setNormalizer(d->normalizerActivated, d->normalizerOption);
		if (d->dirty & ChMap)
			d->mixer->setChannelLayoutMap(d->map);
		if (d->dirty & Scale)
			d->mixer->setScaler(d->tempoScalerActivated, d->scale);
		d->dirty = 0;
	}

	const mp_audio *in = data;
	if (d->resample) {
		const int frames_delay = swr_get_delay(d->swr, data->rate);
		const int frames = av_rescale_rnd(frames_delay + data->samples, d->resampled->rate, data->rate, AV_ROUND_UP);
		mp_audio_realloc_min(d->resampled, frames);
		d->af->delay = (double)frames/data->rate;
		if ((d->resampled->samples = frames))
			d->resampled->samples = swr_convert(d->swr, (uchar**)d->resampled->planes, d->resampled->samples, (const uchar**)data->planes, data->samples);
		in = d->resampled;
	}
	d->mixer->apply(in);
	*data = *d->af->data;
	af->delay += d->mixer->delay();

	if (d->first) {
		emit ac->started();
		d->first = false;
	}
	return 0;
}

void AudioController::setNormalizerActivated(bool on) {
	if (_Change(d->normalizerActivated, on))
		d->dirty |= Normalizer;
}

void AudioController::setTempoScalerActivated(bool on) {
	d->tempoScalerActivated = on;
	d->dirty |= Scale;
}

double AudioController::gain() const {
	return d->normalizerActivated && d->mixer ? d->mixer->gain() : 1.0;
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
	d->dirty |= Normalizer;
}

bool AudioController::isNormalizerActivated() const {
	return d->normalizerActivated;
}

void AudioController::setChannelLayoutMap(const ChannelLayoutMap &map) {
	d->map = map;
	d->dirty |= ChMap;
}

void AudioController::setOutputChannelLayout(ChannelLayout layout) {
	d->layout = layout;
	d->dirty |= ChMap;
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
		AudioController::test,
		sizeof(cmplayer_af_priv),
		nullptr,
		options
	};
	return info;
}
