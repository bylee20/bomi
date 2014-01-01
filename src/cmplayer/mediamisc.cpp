#include "mediamisc.hpp"
#include "hwacc.hpp"
#include "videoformat.hpp"
#include "playengine.hpp"
extern "C" {
#include <player/core.h>
#include <audio/filter/af.h>
#include <video/filter/vf.h>
#include <audio/out/ao.h>
#include <video/decode/dec_video.h>
#include <audio/decode/dec_audio.h>
}

AvInfoObject::AvInfoObject(QObject *parent)
: QObject(parent) {
}

void AvInfoObject::setVideo(const PlayEngine *engine) {
	auto mpctx = engine->context();
	if (!mpctx || !mpctx->d_video)
		return;
	const auto fmt = engine->videoFormat();
	auto dv = mpctx->d_video;

	const auto out = fmt.imgfmt();
	const auto in = dv->vf_input.imgfmt;

	m_codec = _U(dv->decoder_desc);

	m_input->m_type = format(dv->header->format);
	m_input->m_size = QSize(dv->vf_input.d_w, dv->vf_input.d_h);
	m_input->m_fps = dv->fps;
	m_input->m_bps = dv->i_bps*8;
	m_output->m_type = _L(mp_imgfmt_to_name(out));
	if (in != out)
		m_output->m_type = _L(mp_imgfmt_to_name(in)) % _U("→") % m_output->m_type;
	m_output->m_size = fmt.displaySize();
	m_output->m_fps = dv->fps;
	m_output->m_bps = fmt.bps(dv->fps);
}

void AvInfoObject::setAudio(const PlayEngine *engine) {
	auto mpctx = engine->context();
	if (!mpctx || !mpctx->d_audio || !mpctx->ao)
		return;
	auto da = mpctx->d_audio;
	auto ao = mpctx->ao;
	m_audioDriver = AudioDriverInfo::name(engine->audioDriver());
	m_codec = _U(da->decoder_desc);

	const auto out = ao->format;
	const auto in = da->decoded.format;

	m_input->m_type = format(da->header->format);
	m_input->m_bps = da->i_bps*8;
	m_input->m_samplerate = da->decoded.rate/1000.0; // kHz
	m_input->m_channels = QString::fromLatin1(mp_chmap_to_str(&da->decoded.channels));
	m_input->m_bits = af_fmt2bits(da->decoded.format);

	m_output->m_type = _L(af_fmt_to_str(out));
	if (in != out)
		m_output->m_type = _L(af_fmt_to_str(in)) % _U("→") % m_output->m_type;
	m_output->m_bps = ao->bps*8;
	m_output->m_samplerate = ao->samplerate/1000.0;
	m_output->m_channels = QString::fromLatin1(mp_chmap_to_str(&ao->channels));
	m_output->m_bits = af_fmt2bits(ao->format);
}

ChapterInfoObject::ChapterInfoObject(const PlayEngine *engine, QObject *parent)
: TrackInfoObject(parent), m_engine(engine) {
	connect(engine, &PlayEngine::currentChapterChanged, this, &ChapterInfoObject::setCurrent);
}

int ChapterInfoObject::time(int i) const {
	return m_engine->chapters().value(i).time();
}

QString ChapterInfoObject::name(int i) const {
	return m_engine->chapters().value(i).name();
}

AudioTrackInfoObject::AudioTrackInfoObject(const PlayEngine *engine, QObject *parent)
: TrackInfoObject(parent) {
	connect(engine, &PlayEngine::currentAudioStreamChanged, this, &AudioTrackInfoObject::setCurrent);
}
