#include "hwacc_vdpau.hpp"
#include "stdafx.hpp"
#include "videoframe.hpp"

extern "C" {
#include <libavcodec/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <vdpau/vdpau.h>
#include <video/mp_image.h>
#include <video/mp_image_pool.h>
}

#define TO_INTEROP(a) (void*)(quintptr)(a)

void initialize_vdpau_interop(QOpenGLContext *ctx) {
	Vdpau::initializeInterop(ctx);
}

void finalize_vdpau_interop(QOpenGLContext *ctx) {
	Vdpau::finalizeInterop(ctx);
}

const char *HwAccX11Trait<IMGFMT_VDPAU>::error(Status status) {
	if (Vdpau::isInitialized())
		return Vdpau::getErrorString(status);
	return status == success ? "SUCCESS" : "ERROR";
}

void HwAccX11Trait<IMGFMT_VDPAU>::destroySurface(SurfaceID id) {
	if (id != invalid)
		Vdpau::videoSurfaceDestroy(id);
}

bool HwAccX11Trait<IMGFMT_VDPAU>::createSurfaces(int w, int h, int f, QVector<SurfaceID> &ids) {
	VdpauStatusChecker checker;
	for (int i=0; i<ids.size(); ++i) {
		if (!checker.isSuccess(Vdpau::videoSurfaceCreate(f, w, h, &ids[i])))
			return false;
		if (ids[i] == invalid)
			return false;
	}
	return true;
}

Vdpau::Data Vdpau::d;

void Vdpau::initialize() {
	if (d.init)
		return;
	if (!d.check(vdp_device_create_x11(QX11Info::display(), QX11Info::appScreen(), &d.device, &d.proc), "Cannot intialize VDPAU device"))
		return;
	proc(VDP_FUNC_ID_GET_ERROR_STRING, d.getErrorString);
	proc(VDP_FUNC_ID_GET_INFORMATION_STRING, d.getInformationString);
	proc(VDP_FUNC_ID_DEVICE_DESTROY, d.deviceDestroy);
	proc(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES, d.videoSurfaceQueryCapabilities);
	proc(VDP_FUNC_ID_VIDEO_SURFACE_CREATE, d.videoSurfaceCreate);
	proc(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, d.videoSurfaceDestroy);
	proc(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, d.videoSurfaceGetBitsYCbCr);
	proc(VDP_FUNC_ID_DECODER_CREATE, d.decoderCreate);
	proc(VDP_FUNC_ID_DECODER_DESTROY, d.decoderDestroy);
	proc(VDP_FUNC_ID_DECODER_RENDER, d.decoderRender);
	proc(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, d.decoderQueryCapabilities);
	proc(VDP_FUNC_ID_VIDEO_MIXER_CREATE, d.videoMixerCreate);
	proc(VDP_FUNC_ID_VIDEO_MIXER_DESTROY, d.videoMixerDestroy);
	proc(VDP_FUNC_ID_VIDEO_MIXER_RENDER, d.videoMixerRender);
	proc(VDP_FUNC_ID_OUTPUT_SURFACE_CREATE, d.outputSurfaceCreate);
	proc(VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY, d.outputSurfaceDestroy);
	d.init = true;
	if (!d.check(d.status(), "Cannot get VDPAU functions."))
		return;
	auto push = [] (VdpDecoderProfile profile, int avProfile, AVCodecID codec, int surfaces) {
		VdpBool supported = false; quint32 level = 0, blocks = 0, width = 0, height = 0;
		if (d.decoderQueryCapabilities(d.device, profile, &supported, &level, &blocks, &width, &height) != VDP_STATUS_OK || !supported)
			return;
		VdpauProfile p; p.id = profile; p.level = level; p.blocks = blocks; p.width = width; p.height = height;
		auto &supports = d.supports[codec];
		supports.id = codec;
		supports.profiles.push_back(p);
		supports.avProfiles.push_back(avProfile);
		supports.surfaces = surfaces;
	};

	push(VDP_DECODER_PROFILE_MPEG1, FF_PROFILE_UNKNOWN, AV_CODEC_ID_MPEG1VIDEO, 2);
	push(VDP_DECODER_PROFILE_MPEG2_MAIN, FF_PROFILE_MPEG2_MAIN, AV_CODEC_ID_MPEG2VIDEO, 2);
	push(VDP_DECODER_PROFILE_MPEG2_SIMPLE, FF_PROFILE_MPEG2_SIMPLE, AV_CODEC_ID_MPEG2VIDEO, 2);
	push(VDP_DECODER_PROFILE_H264_HIGH, FF_PROFILE_H264_HIGH, AV_CODEC_ID_H264, 16);
	push(VDP_DECODER_PROFILE_H264_BASELINE, FF_PROFILE_H264_BASELINE, AV_CODEC_ID_H264, 16);
	push(VDP_DECODER_PROFILE_H264_BASELINE, FF_PROFILE_H264_CONSTRAINED_BASELINE, AV_CODEC_ID_H264, 16);
	push(VDP_DECODER_PROFILE_H264_MAIN, FF_PROFILE_H264_MAIN, AV_CODEC_ID_H264, 16);
	push(VDP_DECODER_PROFILE_VC1_ADVANCED, FF_PROFILE_VC1_ADVANCED, AV_CODEC_ID_VC1, 2);
	push(VDP_DECODER_PROFILE_VC1_SIMPLE, FF_PROFILE_VC1_SIMPLE, AV_CODEC_ID_VC1, 2);
	push(VDP_DECODER_PROFILE_VC1_MAIN, FF_PROFILE_VC1_MAIN, AV_CODEC_ID_VC1, 2);
	push(VDP_DECODER_PROFILE_VC1_MAIN, FF_PROFILE_VC1_MAIN, AV_CODEC_ID_WMV3, 2);
	push(VDP_DECODER_PROFILE_VC1_SIMPLE, FF_PROFILE_VC1_SIMPLE, AV_CODEC_ID_WMV3, 2);
	push(VDP_DECODER_PROFILE_VC1_ADVANCED, FF_PROFILE_VC1_ADVANCED, AV_CODEC_ID_WMV3, 2);
	push(VDP_DECODER_PROFILE_MPEG4_PART2_ASP, FF_PROFILE_MPEG4_ADVANCED_SIMPLE, AV_CODEC_ID_MPEG4, 2);
	push(VDP_DECODER_PROFILE_MPEG4_PART2_SP, FF_PROFILE_MPEG4_SIMPLE, AV_CODEC_ID_MPEG4, 2);
	_Debug("VDPAU device is initialized.");
}

void Vdpau::finalize() {
	if (d.init) {
		deviceDestroy();
		d.init = false;
	}
}

void Vdpau::initializeInterop(QOpenGLContext *ctx) {
	if (!d.init || d.initialize)
		return;
	d.gl = ctx;
	proc("glVDPAUInitNV", d.initialize);
	proc("glVDPAUFiniNV", d.finalize);
	proc("glVDPAURegisterOutputSurfaceNV", d.registerOutputSurface);
	proc("glVDPAUIsSurfaceNV", d.isSurface);
	proc("glVDPAUUnregisterSurfaceNV", d.unregisterSurface);
	proc("glVDPAUGetSurfaceivNV", d.getSurfaceiv);
	proc("glVDPAUSurfaceAccessNV", d.surfaceAccess);
	proc("glVDPAUMapSurfacesNV", d.mapSurfaces);
	proc("glVDPAUUnmapSurfacesNV", d.unmapSurfaces);
	d.initialize(TO_INTEROP(d.device), TO_INTEROP(d.proc));
}



void Vdpau::finalizeInterop(QOpenGLContext *ctx) {
	if (d.initialize) {
		Q_ASSERT(ctx == d.gl);
		d.finalize();
	}
}

struct VdpauVideoSurface {
	VdpVideoSurface id = VDP_INVALID_HANDLE;
	bool ref = false;
};

struct HwAccVdpau::Data {
	AVVDPAUContext context;
	QSize avSize = {0, 0};
	VdpauSurfacePool pool;
};

HwAccVdpau::HwAccVdpau(AVCodecID cid)
: HwAcc(cid), d(new Data) {
	memset(&d->context, 0, sizeof(d->context));
	d->context.decoder = VDP_INVALID_HANDLE;
}

HwAccVdpau::~HwAccVdpau() {
	freeContext();
	delete d;
}

void HwAccVdpau::freeContext() {
	if (d->context.decoder != VDP_INVALID_HANDLE) {
		Vdpau::decoderDestroy(d->context.decoder);
		d->context.decoder = VDP_INVALID_HANDLE;
	}
}

bool HwAccVdpau::fillContext(AVCodecContext *avctx) {
	if (!isSuccess())
		return false;
	freeContext();
	auto codec = Vdpau::codec(avctx->codec_id);
	if (!codec)
		return false;
	const auto profile = codec->profile(avctx->profile);
	if (profile.width < avctx->width || profile.height < avctx->height || profile.level < avctx->level)
		return false;
	VdpBool supports = false; uint mwidth = 0, mheight = 0;
	constexpr VdpChromaType chroma = VDP_CHROMA_TYPE_420;
	if (!isSuccess(Vdpau::videoSurfaceQueryCapabilities(chroma, &supports, &mwidth, &mheight)))
		return false;
	if (!supports || (int)mwidth < avctx->width || (int)mheight < avctx->height)
		return false;
	const int width = (avctx->width + 1) & ~1;
	const int height = (avctx->height + 3) & ~3;
	if (!isSuccess(Vdpau::decoderCreate(profile.id, width, height, codec->surfaces, &d->context.decoder))) {
		d->context.decoder = VDP_INVALID_HANDLE;
		return false;
	}
	if (!d->pool.create(codec->surfaces + 1, width, height, chroma))
		return false;
	d->avSize = QSize(avctx->width, avctx->height);
	d->context.render = Vdpau::decoderRender;
	return true;
}

mp_image *HwAccVdpau::getSurface() {
	return d->pool.getMpImage();
}

void *HwAccVdpau::context() const {
	return &d->context;
}

mp_image *HwAccVdpau::getImage(mp_image *mpi) {
	return mpi;
}

/******************************************************************/

VdpauMixer::VdpauMixer(const OpenGLTexture &texture, const VideoFormat &format)
: m_width(format.width()), m_height(format.height()) {
	static const QVector<VdpVideoMixerParameter> params = {
		VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
		VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
		VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE,
	};
	const QVector<void *> values = { &m_width, &m_height, &m_chroma };
	if (!check(Vdpau::videoMixerCreate(0, nullptr, params.size(), params.data(), values.data(), &m_mixer), "Cannot create videp mixer."))
		return;
	if (!check(Vdpau::outputSurfaceCreate(VDP_RGBA_FORMAT_B8G8R8A8, m_width, m_height, &m_surface), "Cannot create output surface."))
		return;
	m_glSurface = Vdpau::registerOutputSurface(m_surface, texture.target, 1, &texture.id);
	if (m_glSurface == GL_NONE && !check(VDP_STATUS_ERROR, "Cannot register output surface."))
		return;
	Vdpau::surfaceAccess(m_glSurface, GL_READ_ONLY);
	Vdpau::mapSurfaces(1, &m_glSurface);
	_Debug("VDPAU Mixer initialized.");
}

VdpauMixer::~VdpauMixer() {
	if (m_glSurface) {
		Vdpau::unmapSurfaces(1, &m_glSurface);
		Vdpau::unregisterSurface(m_glSurface);
	}
	if (m_surface != VDP_INVALID_HANDLE)
		Vdpau::outputSurfaceDestroy(m_surface);
	if (m_mixer != VDP_INVALID_HANDLE)
		Vdpau::videoMixerDestroy(m_mixer);
}

bool VdpauMixer::upload(VideoFrame &frame, bool deint) {
	static const VdpVideoMixerPictureStructure structures[] = {
		// Picture = 0,   Top = 1,      Bottom = 2
		VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD
	};
	VdpVideoMixerPictureStructure structure = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
	if (deint)
		structure = structures[frame.field() & VideoFrame::Interlaced];
	const auto id = (VdpVideoSurface)(quintptr)(frame.data(3));
	return check(Vdpau::videoMixerRender(m_mixer, VDP_INVALID_HANDLE, nullptr, structure, 0, nullptr, id, 0, nullptr
		, nullptr, m_surface, nullptr, nullptr, 0, nullptr), "Cannot render video surface.");
}
