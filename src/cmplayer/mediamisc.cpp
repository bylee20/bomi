#include "mediamisc.hpp"
#include "hwacc.hpp"
#include "videoformat.hpp"
#include "playengine.hpp"
extern "C" {
#include <mpvcore/mp_core.h>
#include <audio/filter/af.h>
#include <video/filter/vf.h>
#include <audio/out/ao.h>
}

AvInfoObject::AvInfoObject(QObject *parent)
: QObject(parent) {
}

void AvInfoObject::setVideo(const PlayEngine *engine) {
	auto mpctx = engine->context();
	if (!mpctx || !mpctx->sh_video || !mpctx->sh_video)
		return;
	const auto fmt = engine->videoFormat();
	auto sh = mpctx->sh_video;

	const auto out = fmt.imgfmt();
	const auto in = sh->vfilter ? sh->vfilter->fmt_in.params.imgfmt : out;

	m_codec = _U(mpctx->sh[STREAM_VIDEO]->decoder_desc);

	m_input->m_type = format(sh->format);
	m_input->m_size = QSize(sh->disp_w, sh->disp_h);
	m_input->m_fps = sh->fps;
	m_input->m_bps = sh->i_bps*8;
	m_output->m_type = _L(mp_imgfmt_to_name(out));
	if (in != out)
		m_output->m_type = _L(mp_imgfmt_to_name(in)) % _U("→") % m_output->m_type;
	m_output->m_size = fmt.displaySize();
	m_output->m_fps = sh->fps;
	m_output->m_bps = fmt.bps(sh->fps);
}

void AvInfoObject::setAudio(const PlayEngine *engine) {
	auto mpctx = engine->context();
	if (!mpctx || !mpctx->sh_audio || !mpctx->ao)
		return;
	auto sh = mpctx->sh_audio;
	auto ao = mpctx->ao;
	m_audioDriver = AudioDriverInfo::name(engine->audioDriver());
	m_codec = _U(mpctx->sh[STREAM_AUDIO]->decoder_desc);

	const auto out = ao->format;
	const auto in = sh->afilter ? sh->afilter->input.format : out;

	m_input->m_type = format(sh->format);
	m_input->m_bps = sh->i_bps*8;
	m_input->m_samplerate = sh->samplerate/1000.0; // kHz
	m_input->m_channels = QString::fromLatin1(mp_chmap_to_str(&sh->channels));
	m_input->m_bits = af_fmt2bits(sh->sample_format);

	m_output->m_type = _L(af_fmt2str_short(out));
	if (in != out)
		m_output->m_type = _L(af_fmt2str_short(in)) % _U("→") % m_output->m_type;
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
