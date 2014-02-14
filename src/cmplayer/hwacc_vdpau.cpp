#include "hwacc_vdpau.hpp"
#include "stdafx.hpp"

extern "C" {
#include <libavcodec/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <vdpau/vdpau.h>
#include <video/mp_image.h>
#include <video/mp_image_pool.h>
}

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

Vdpau::Data Vdpau::d;

void Vdpau::initialize() {
	if (d.init)
		return;
	if (!d.isSuccess(vdp_device_create_x11(QX11Info::display(), QX11Info::appScreen(), &d.device, &d.proc))) {
		_Error("Cannot intialize VDPAU device");
		return;
	}
	proc(VDP_FUNC_ID_GET_ERROR_STRING, d.getErrorString);
	proc(VDP_FUNC_ID_GET_INFORMATION_STRING, d.getInformationString);
	proc(VDP_FUNC_ID_DEVICE_DESTROY, d.deviceDestroy);
	proc(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES, d.videoSurfaceQueryCapabilities);
	proc(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES, d.videoSurfaceQueryGetPutBitsYCbCrCapabilities);
	proc(VDP_FUNC_ID_VIDEO_SURFACE_CREATE, d.videoSurfaceCreate);
	proc(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, d.videoSurfaceDestroy);
	proc(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, d.videoSurfaceGetBitsYCbCr);
	proc(VDP_FUNC_ID_DECODER_CREATE, d.decoderCreate);
	proc(VDP_FUNC_ID_DECODER_DESTROY, d.decoderDestroy);
	proc(VDP_FUNC_ID_DECODER_RENDER, d.decoderRender);
	proc(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, d.decoderQueryCapabilities);
	d.init = true;
	if (!d.isSuccess()) {
		_Error("Cannot get VDPAU functions.");
		return;
	}
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
	if (d.init && !d.initialize) {
		d.gl = ctx;
		proc("glVDPAUInitNV", d.initialize);
		proc("glVDPAUFiniNV", d.finalize);
		proc("glVDPAURegisterVideoSurfaceNV", d.registerVideoSurface);
		proc("glVDPAURegisterOutputSurfaceNV", d.registerOutputSurface);
		proc("glVDPAUIsSurfaceNV", d.isSurface);
		proc("glVDPAUUnregisterSurfaceNV", d.unregisterSurface);
		proc("glVDPAUGetSurfaceivNV", d.getSurfaceiv);
		proc("glVDPAUSurfaceAccessNV", d.surfaceAccess);
		proc("glVDPAUMapSurfacesNV", d.mapSurfaces);
		proc("glVDPAUUnmapSurfacesNV", d.unmapSurfaces);
		d.initialize(reinterpret_cast<void*>(d.device), reinterpret_cast<void*>(d.proc));
	}
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
	QVector<VdpauVideoSurface> surfaces;
	QVector<VdpauVideoSurface>::iterator it;
	mp_image_pool *pool = nullptr;
	QSize surfaceSize = {0, 0};
	VdpYCbCrFormat format = VdpYCbCrFormat(-1);
	mp_imgfmt imgfmt = IMGFMT_NONE;
};

HwAccVdpau::HwAccVdpau(AVCodecID cid)
: HwAcc(cid), d(new Data) {
	memset(&d->context, 0, sizeof(d->context));
	d->context.decoder = VDP_INVALID_HANDLE;
	d->it = d->surfaces.end();
	d->pool = mp_image_pool_new(5);
}

HwAccVdpau::~HwAccVdpau() {
	freeContext();
	if (d->pool)
		mp_image_pool_clear(d->pool);
	delete d;
}

void HwAccVdpau::freeContext() {
	for (auto it = d->surfaces.begin(); it != d->surfaces.end(); ++it) {
		if (it->id != VDP_INVALID_HANDLE)
			Vdpau::videoSurfaceDestroy(it->id);
	}
	d->surfaces.clear();
	d->it = d->surfaces.end();
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
	const VdpChromaType chroma = VDP_CHROMA_TYPE_420;
	if (!isSuccess(Vdpau::videoSurfaceQueryCapabilities(chroma, &supports, &mwidth, &mheight)))
		return false;
	if (!supports || (int)mwidth < avctx->width || (int)mheight < avctx->height)
		return false;
	VdpYCbCrFormat format;
	auto check = [this, &format, &supports] (VdpYCbCrFormat f) {
		if (!isSuccess(Vdpau::videoSurfaceQueryGetPutBitsYCbCrCapabilities(chroma, format = f, &supports)))
			return false;
		return (bool)supports;
	};
	if (!check(VDP_YCBCR_FORMAT_YV12) && !check(VDP_YCBCR_FORMAT_NV12))
		return false;
	Q_ASSERT(format == VDP_YCBCR_FORMAT_YV12 || format == VDP_YCBCR_FORMAT_NV12);
	Q_ASSERT(supports == true);
	if (format == VDP_YCBCR_FORMAT_YV12)
		d->imgfmt = IMGFMT_420P;
	else
		d->imgfmt = IMGFMT_NV12;
	const int width = (avctx->width + 1) & ~1;
	const int height = (avctx->height + 3) & ~3;
	if (!isSuccess(Vdpau::decoderCreate(profile.id, width, height, codec->surfaces, &d->context.decoder))) {
		d->context.decoder = VDP_INVALID_HANDLE;
		return false;
	}
	d->surfaces.resize(codec->surfaces + 1);
	for (auto it = d->surfaces.begin(); it != d->surfaces.end(); ++it) {
		if (!isSuccess(Vdpau::videoSurfaceCreate(chroma, width, height, &it->id)))
			return false;
		if (it->id == VDP_INVALID_HANDLE)
			return false;
	}
	d->format = format;
	d->surfaceSize = QSize(width, height);
	d->it = d->surfaces.begin();
	d->avSize = QSize(avctx->width, avctx->height);
	d->context.render = Vdpau::decoderRender;
	return true;
}

mp_image *HwAccVdpau::getSurface() {
	if (d->surfaces.isEmpty())
		return nullptr;
	if (++d->it == d->surfaces.end())
		d->it = d->surfaces.begin();
	Q_ASSERT(!d->it->ref);
	Q_ASSERT(d->it->id != VDP_INVALID_HANDLE);
	d->it->ref = true;
	auto release = [] (void *arg) {*(bool*)arg = false;};
	auto mpi = nullMpImage(IMGFMT_VDPAU, d->avSize.width(), d->avSize.height(), &d->it->ref, release);
	mpi->planes[0] = mpi->planes[3] = (uchar*)(void*)(uintptr_t)(d->it->id);
	mpi->planes[1] = mpi->planes[2] = nullptr;
	return mpi;
}

void *HwAccVdpau::context() const {
	return &d->context;
}

mp_image *HwAccVdpau::getImage(mp_image *mpi) {
	return mpi;
	const auto id = (VdpVideoSurface)(uintptr_t)(void*)mpi->planes[3];
	auto img = mp_image_pool_get(d->pool, d->imgfmt, d->avSize.width(), d->avSize.height());
	void *data[] = { img->planes[0], img->planes[1], img->planes[2] };
	quint32 pitches[] = { (quint32)img->stride[0], (quint32)img->stride[1], (quint32)img->stride[2] };
	if (!isSuccess(Vdpau::videoSurfaceGetBitsYCbCr(id, d->format, data, pitches))) {
//		return nullptr;
	}
	img->pts = mpi->pts;
//	if (d->format == VDP_YCBCR_FORMAT_YV12)
//		qSwap(img->planes[1], img->planes[2]);
	return img;
}

