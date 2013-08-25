#include "playinfoitem.hpp"
#include "hwacc.hpp"
#include "videoformat.hpp"
#include "playengine.hpp"
extern "C" {
#include "mpvcore/mp_core.h"
#include "audio/filter/af.h"
#include "audio/out/ao.h"
}

void AvInfoObject::setHwAcc(PlayerItem::HwAcc acc) {
	switch (m_hwAcc = acc) {
	case PlayerItem::Activated:
		m_hwAccText = tr("Activated");
		break;
	case PlayerItem::Inactivated:
		m_hwAccText = tr("Inactivated");
		break;
	default:
		m_hwAccText = tr("Unavailable");
		break;
	}
}

void AvInfoObject::setVideo(const PlayEngine *engine) {
	auto mpctx = engine->context();
	if (!mpctx || !mpctx->sh_video || !mpctx->sh_video)
		return;
	const auto fmt = engine->videoFormat();
	auto sh = mpctx->sh_video;

	if (!HwAcc::supports(HwAcc::codecId(mpctx->sh[STREAM_VIDEO]->codec)))
		setHwAcc(PlayerItem::Unavailable);
	else if (engine->isHwAccActivated())
		setHwAcc(PlayerItem::Activated);
	else
		setHwAcc(PlayerItem::Inactivated);
	m_codec = _U(mpctx->sh[STREAM_VIDEO]->decoder_desc);

	m_input->m_type = format(sh->format);
	m_input->m_size = QSize(sh->disp_w, sh->disp_h);
	m_input->m_fps = sh->fps;
	m_input->m_bps = sh->i_bps*8;
	m_output->m_type = fmt.name();
	m_output->m_size = fmt.outputSize();
	m_output->m_fps = sh->fps;
	m_output->m_bps = fmt.bps(sh->fps);
}

void AvInfoObject::setAudio(const PlayEngine *engine) {
	auto mpctx = engine->context();
	if (!mpctx || !mpctx->sh_audio || !mpctx->ao)
		return;
	auto sh = mpctx->sh_audio;
	auto ao = mpctx->ao;
	m_hwAcc = PlayerItem::Unavailable;
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
