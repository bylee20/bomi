#include "hwaccel.hpp"

extern int HWACCEL_FORMAT;
extern "C" void *fast_memcpy(void *to, const void *from, size_t len);

void *cmplayer_hwaccel_create(AVCodecContext *avctx) {
	return HwAccel::create(avctx);
}

void cmplayer_hwaccel_delete(void **hwaccel) {
	if (*hwaccel) {
		delete reinterpret_cast<HwAccel*>(*hwaccel);
		*hwaccel = nullptr;
	}
}

HwAccel *HwAccel::create(AVCodecContext *avctx) {
	if (!HWACCEL_FORMAT)
		return nullptr;
	switch (avctx->codec->id) {
	case CODEC_ID_H264:
		break;
	default:
		return nullptr;
	}
	avctx->get_format = getFormat;
	avctx->get_buffer = getBuffer;
	avctx->release_buffer = releaseBuffer;
	avctx->reget_buffer = regetBuffer;
	avctx->draw_horiz_band = nullptr;
	auto hwaccel = new HwAccel;
	hwaccel->avctx = avctx;
	return hwaccel;
}

HwAccel::~HwAccel() {
	if (avctx) {
		dtorContext();
		avctx->hwaccel_context = nullptr;
	}
}

bool HwAccel::ctorContext(AVCodecContext *avctx) {
	memset(&ctx, 0, sizeof(ctx));
	width = avctx->width;
	height = avctx->height;
	if (width <= 0 || height <= 0)
		return false;
#ifdef Q_WS_MAC
	ctx.decoder = nullptr;
	ctx.width = avctx->width;
	ctx.height = avctx->height;
	ctx.format = 'avc1';
	ctx.cv_pix_fmt_type = HWACCEL_FORMAT == IMGFMT_YUY2 ? kCVPixelFormatType_422YpCbCr8_yuvs : kCVPixelFormatType_420YpCbCr8Planar;
	ctx.use_sync_decoding = 1;
	imgFmt = HWACCEL_FORMAT;
	return ff_vda_create_decoder(&ctx, avctx->extradata, avctx->extradata_size) == 0;
#else
	VAProfile profile;
	switch(avctx->codec_id) {
	case CODEC_ID_MPEG1VIDEO:
	case CODEC_ID_MPEG2VIDEO:
		profile = VAProfileMPEG2Main;
		surfaces.resize(2+1);
		break;
	case CODEC_ID_MPEG4:
		profile = VAProfileMPEG4AdvancedSimple;
		surfaces.resize(2+1);
		break;
	case CODEC_ID_WMV3:
		profile = VAProfileVC1Main;
		surfaces.resize(2+1);
	case CODEC_ID_VC1:
		profile = VAProfileVC1Advanced;
		surfaces.resize(2+1);
	case CODEC_ID_H264:
		profile = VAProfileH264High;
		surfaces.resize(16+1);
		break;
	default:
		return false;
	}
	ctx.config_id = ctx.context_id = VA_INVALID_ID;
	if (!(x11 = XOpenDisplay(nullptr)))
		return false;
	if (!(ctx.display = vaGetDisplay(x11)))
		return false;
	int major, minor;
	if (vaInitialize(ctx.display, &major, &minor))
		return false;
	auto profileCount = vaMaxNumProfiles(ctx.display);
	QVector<VAProfile> profiles(profileCount);
	if (vaQueryConfigProfiles(ctx.display, profiles.data(), &profileCount))
		return false;
	auto supports = false;
	for (int i=0; i<profileCount; ++i) {
		if ((supports = (profile == profiles[i])))
			break;
	}
	if (!supports)
		return false;
	VAConfigAttrib attr;
	memset(&attr, 0, sizeof(attr));
	attr.type = VAConfigAttribRTFormat;
	if(vaGetConfigAttributes(ctx.display, profile, VAEntrypointVLD, &attr, 1))
		return false;
	if(!(attr.value & VA_RT_FORMAT_YUV420))
		return false;
	if(vaCreateConfig(ctx.display, profile, VAEntrypointVLD, &attr, 1, &ctx.config_id)) {
		ctx.config_id = VA_INVALID_ID;
		return false;
	}

	QVector<VASurfaceID> ids(surfaces.size());
	if (vaCreateSurfaces(ctx.display, width, height, VA_RT_FORMAT_YUV420, ids.size(), ids.data())) {
		for (auto &surface : surfaces)
			surface.id = VA_INVALID_SURFACE;
		return false;
	}
	for (int i=0; i<surfaces.size(); ++i)
		surfaces[i].id = ids[i];
	if (vaCreateContext(ctx.display, ctx.config_id, width, height, VA_PROGRESSIVE, ids.data(), ids.size(), &ctx.context_id)) {
		ctx.context_id = VA_INVALID_ID;
		return false;
	}
	int formatCount = vaMaxNumImageFormats(ctx.display);
	QVector<VAImageFormat> formats(formatCount);
	if (vaQueryImageFormats(ctx.display, formats.data(), &formatCount))
		return false;
	imgFmt = 0;
	VAImageFormat format;
	image.image_id = VA_INVALID_ID;
	for (int i=0; i<formatCount && !imgFmt; ++i) {
		auto &fmt = formats[i];
		uint32_t fourcc = 0;
		switch (fmt.fourcc) {
		case VA_FOURCC('Y', 'V', '1', '2'):
		case VA_FOURCC('I', '4', '2', '0'):
			fourcc = IMGFMT_YV12;
			break;
		case VA_FOURCC('N', 'V', '1', '2'):
			fourcc = IMGFMT_NV12;
			break;
		case VA_FOURCC('N', 'V', '2', '1'):
			fourcc = IMGFMT_NV21;
			break;
		default:
			break;
		}
		if (!fourcc)
			continue;
		if (vaCreateImage(ctx.display, &fmt, width, height, &image)) {
			image.image_id = VA_INVALID_ID;
			continue;
		}
		if (vaGetImage(ctx.display, ids[0], 0, 0, width, height, image.image_id)) {
			vaDestroyImage(ctx.display, image.image_id);
			image.image_id = VA_INVALID_ID;
			continue;
		}
		imgFmt = fourcc;
		format = fmt;
		break;
	}
	if (!imgFmt)
		return false;

//	CopyInitCache( &p_va->image_cache, i_width );

	/* Setup the ffmpeg hardware context */
//	p_va->i_surface_chroma = i_chroma;
	return true;
#endif
}

void HwAccel::dtorContext() {
#ifdef Q_WS_MAC
		if (ctx.decoder)
			ff_vda_destroy_decoder(&ctx);
		memset(&ctx, 0, sizeof(ctx));
#else
	if (ctx.display) {
		if (image.image_id != VA_INVALID_ID) {
	//		CopyCleanCache( &p_va->image_cache );
			vaDestroyImage(ctx.display, image.image_id);
		}
		if (ctx.context_id != VA_INVALID_ID)
			vaDestroyContext(ctx.display, ctx.context_id);
		for (auto &surface : surfaces) {
			if (surface.id != VA_INVALID_SURFACE)
				vaDestroySurfaces(ctx.display, &surface.id, 1);
		}
		surfaces.clear();
		if (ctx.config_id != VA_INVALID_ID)
			vaDestroyConfig(ctx.display, ctx.config_id);
		vaTerminate(ctx.display);
	}
	if (x11)
		XCloseDisplay(x11);
	memset(&ctx, 0, sizeof(ctx));
	ctx.config_id = ctx.context_id = image.image_id = VA_INVALID_ID;
#endif
	width = height = 0;
}

uint32_t cmplayer_hwaccel_setup(void *hwaccel) {
	auto hwa = reinterpret_cast<HwAccel*>(hwaccel);
	return hwa->setup() ? hwa->imgFmt : 0;
}

bool HwAccel::setup() {
	if (initCtx && avctx->width == width && avctx->height == height)
		return true;
	dtorContext();
	if ((initCtx = ctorContext(avctx))) {
		avctx->hwaccel_context = &ctx;
		return true;
	} else {
		avctx->hwaccel_context = nullptr;
		dtorContext();
		return false;
	}
}

PixelFormat HwAccel::getFormat(AVCodecContext *avctx, const PixelFormat *pixFmt) {
	auto hwaccel = get(avctx);
	if (!hwaccel)
		return PIX_FMT_NONE;
	auto sh = _sh(avctx);
#ifdef Q_WS_MAC
	const auto format = PIX_FMT_VDA_VLD;
#else
	const auto format = PIX_FMT_VAAPI_VLD;
#endif
	int i = 0;
	for (; pixFmt[i] != PIX_FMT_NONE; ++i) {
		if (format == pixFmt[i] && init_vo(sh, pixFmt[i]) >= 0 && hwaccel->setup()) {
			qDebug() << "Found HwAccel!";
			break;
		}
	}
	return pixFmt[i];
}

int HwAccel::getBuffer(AVCodecContext *avctx, AVFrame *frame) {
	auto hwaccel = get(avctx);
	Q_ASSERT(hwaccel->avctx == avctx);
	frame->reordered_opaque = avctx->reordered_opaque;
	frame->opaque = nullptr;
	frame->type = FF_BUFFER_TYPE_USER;
	for (int i=0; i<4; ++i) {
		frame->data[i] = nullptr;
		frame->linesize[i] = 0;
	}
	frame->type = FF_BUFFER_TYPE_USER;
	hwaccel->setup();
#ifdef Q_WS_MAC
	auto dummy = reinterpret_cast<uint8_t*>(1);
#else
	int i = 0, old = 0;
	for (; i<hwaccel->surfaces.size(); ++i) {
		auto &surface = hwaccel->surfaces[i];
		if (!surface.ref)
			break;
		if (surface.order < hwaccel->surfaces[old].order)
			old = i;
	}
	if (i >= hwaccel->surfaces.size())
		i = old;
	auto surface = &hwaccel->surfaces[i];
	surface->ref = 1;
	surface->order = ++hwaccel->totalOrder;
	auto dummy = reinterpret_cast<uint8_t*>(surface->id);
#endif
	frame->data[0] = frame->data[3] = dummy;
	return 0;
}

int HwAccel::regetBuffer(AVCodecContext *avctx, AVFrame *frame) {
	frame->reordered_opaque = avctx->reordered_opaque;
	return avcodec_default_reget_buffer(avctx, frame);
}

void HwAccel::releaseBuffer(AVCodecContext *avctx, AVFrame *frame) {
#ifdef Q_WS_MAC
	(void)avctx;
	auto cv_buffer = reinterpret_cast<CVPixelBufferRef>(frame->data[3]);
	if (cv_buffer)
		CFRelease(cv_buffer);
#else
	auto id = (VASurfaceID)(uintptr_t)frame->data[3];
	auto hwaccel = get(avctx);
	for (auto &surface : hwaccel->surfaces) {
		if (surface.id == id)
			--surface.ref;
	}
#endif
	frame->data[0] = frame->data[1] = frame->data[2] = frame->data[3] = nullptr;
}

bool HwAccel::fill(mp_image_t *image, AVFrame *frame) {
#ifdef Q_WS_MAC
	CVPixelBufferRef buffer = (CVPixelBufferRef)(frame->data[3]);
	CVPixelBufferLockBaseAddress(buffer, 0);
	image->planes[0] = (uchar*)CVPixelBufferGetBaseAddressOfPlane(buffer, 0);
	image->planes[1] = (uchar*)CVPixelBufferGetBaseAddressOfPlane(buffer, 1);
	image->planes[2] = (uchar*)CVPixelBufferGetBaseAddressOfPlane(buffer, 2);
	image->stride[0] = CVPixelBufferGetBytesPerRowOfPlane(buffer, 0);
	image->stride[1] = CVPixelBufferGetBytesPerRowOfPlane(buffer, 1);
	image->stride[2] = CVPixelBufferGetBytesPerRowOfPlane(buffer, 2);
	CVPixelBufferUnlockBaseAddress(buffer, 0);
#else
	VASurfaceID id = (VASurfaceID)(uintptr_t)frame->data[3];
#if VA_CHECK_VERSION(0,31,0)
	if (vaSyncSurface(ctx.display, id))
#else
	if (vaSyncSurface(ctx.display, ctx.context_id, id))
#endif
		return false;

	/* XXX vaDeriveImage may be better but it is not supported by
	* my setup.
	*/
	if (vaGetImage(ctx.display, id, 0, 0, width, height, this->image.image_id))
		return false;
	void *buffer = nullptr;
	if (vaMapBuffer(ctx.display, this->image.buf, &buffer))
		return false;
	const auto fourcc = this->image.format.fourcc;
	auto copyPlane = [this, buffer] (QByteArray &dest, int idx, size_t len) {
		dest.resize(len);
		fast_memcpy(dest.data(), (uchar*)buffer + this->image.offsets[idx], len);
	};

	const int stride = this->image.pitches[0];
	const size_t len = stride*height;
	int idx_u = 1, idx_v = 2;
	switch (fourcc) {
	case VA_FOURCC('I', '4', '2', '0'):
		idx_u = 2; idx_v = 1;
	case VA_FOURCC('Y', 'V', '1', '2'):
		copyPlane(this->buffer[0], 0, len);
		copyPlane(this->buffer[1], idx_u, len >> 2);
		copyPlane(this->buffer[2], idx_v, len >> 2);
		image->planes[0] = (uchar*)this->buffer[0].data();
		image->planes[1] = (uchar*)this->buffer[1].data();
		image->planes[2] = (uchar*)this->buffer[2].data();
		image->stride[0] = this->buffer[0].size();
		image->stride[1] = this->buffer[1].size();
		image->stride[2] = this->buffer[2].size();
		break;
	case VA_FOURCC('N', 'V', '1', '2'):
	case VA_FOURCC('N', 'V', '2', '1'):
		copyPlane(this->buffer[0], 0, len);
		copyPlane(this->buffer[1], 1, len >> 1);
		image->planes[0] = (uchar*)this->buffer[0].data();
		image->planes[1] = (uchar*)this->buffer[1].data();
		image->stride[0] = this->buffer[0].size();
		image->stride[1] = this->buffer[1].size();
		break;
	default:
		return false;
	}
	if(vaUnmapBuffer(ctx.display, this->image.buf))
		return false;
#endif
	return true;
}

int cmplayer_hwaccel_fill_image(void *hwaccel, mp_image_t *image, AVFrame *frame) {
	return reinterpret_cast<HwAccel*>(hwaccel)->fill(image, frame);
}
