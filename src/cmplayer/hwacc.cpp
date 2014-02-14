#include "hwacc.hpp"
#include "videooutput.hpp"
#include "stdafx.hpp"
#include "hwacc_vaapi.hpp"
#include "hwacc_vdpau.hpp"
#include "hwacc_vda.hpp"
extern "C" {
#include <video/decode/lavc.h>
#include <video/decode/dec_video.h>
#include <common/av_common.h>
}

static mp_image null;

mp_image *nullMpImage(void *arg, void(*free)(void*)) { return mp_image_new_custom_ref(&null, arg, free); }

mp_image *nullMpImage(uint imgfmt, int width, int height, void *arg, void(*free)(void*)) {
	auto mpi = nullMpImage(arg, free); mp_image_setfmt(mpi, imgfmt); mp_image_set_size(mpi, width, height); return mpi;
}

bool HwAcc::supports(AVCodecID codec) {
#ifdef Q_OS_MAC
	return codec == AV_CODEC_ID_H264;
#endif
#ifdef Q_OS_LINUX
	return Vdpau::codec(codec) != nullptr;
	return VaApi::codec(codec) != nullptr;
#endif
}

bool HwAcc::supports(DeintMethod method) {
#ifdef Q_OS_MAC
	Q_UNUSED(method);
	return false;
#endif
#ifdef Q_OS_LINUX
#ifdef USE_VAVPP
	auto filter = VaApi::filter(VAProcFilterDeinterlacing);
	return filter && filter->supports(VaApi::toVAType(method));
#else
	return method == DeintMethod::Bob;
#endif
#endif
}

QList<DeintMethod> HwAcc::fullDeintList() {
	static auto list = QList<DeintMethod>() << DeintMethod::Bob;
	return list;
}

void HwAcc::initialize() {
	VaApi::initialize();
	Vdpau::initialize();
}

void HwAcc::finalize() {
	VaApi::finalize();
	Vdpau::finalize();
}

struct CodecInfo {
	CodecInfo(AVCodecID id = AV_CODEC_ID_NONE, const char *name = "unknown")
	: id(id), name(name) {}
	AVCodecID id; const char *name;
};

static const CodecInfo codecs[] = {
	{AV_CODEC_ID_MPEG1VIDEO,	"mpeg1video"},
	{AV_CODEC_ID_MPEG2VIDEO,	"mpeg2video"},
	{AV_CODEC_ID_MPEG4,			"mpeg4"},
	{AV_CODEC_ID_WMV3,			"wmv3"},
	{AV_CODEC_ID_VC1,			"vc1"},
	{AV_CODEC_ID_H264,			"h264"}
};

const char *HwAcc::codecName(int id) {
	for (auto &info : codecs) {
		if (info.id == id)
			return info.name;
	}
	return "unknow";
}

AVCodecID HwAcc::codecId(const char *name) {
	if (!name)
		return AV_CODEC_ID_NONE;
	for (auto &info : codecs) {
		if (qstrcmp(info.name, name) == 0)
			return info.id;
	}
	return AV_CODEC_ID_NONE;
}

QList<AVCodecID> HwAcc::fullCodecList() {
	static QList<AVCodecID> list;
	if (list.isEmpty()) {
		for (auto &info : codecs)
			list << info.id;
	}
	 return list;
}

struct HwAcc::Data {
	mp_image nullImage;
	int imgfmt = IMGFMT_NONE;
};

HwAcc::HwAcc(AVCodecID codec)
: d(new Data), m_codec(codec) {
	memset(&d->nullImage, 0, sizeof(d->nullImage));
}

HwAcc::~HwAcc() {
	delete d;
}

VideoOutput *HwAcc::vo(lavc_ctx *ctx) {
	return static_cast<VideoOutput*>((void*)(ctx->hwdec_info->vdpau_ctx));
}

int HwAcc::imgfmt() const {
	return d->imgfmt;
}

int HwAcc::init(lavc_ctx *ctx) {
	auto format = ctx->hwdec->image_formats;
	if (!format)
		return -1;
	HwAcc *acc = nullptr;
#ifdef Q_OS_LINUX
	if (format[0] == IMGFMT_VAAPI)
		acc = new HwAccVaApi(ctx->avctx->codec_id);
	else if (format[0] == IMGFMT_VDPAU)
		acc = new HwAccVdpau(ctx->avctx->codec_id);
#endif
#ifdef Q_OS_MAC
	if (format[0] == IMGFMT_VDA)
		acc = new HwAccVda(ctx->avctx->codec_id);
#endif
	if (!acc || !acc->isOk()) {
		delete acc;
		return -1;
	}
	acc->d->imgfmt = format[0];
	vo(ctx)->setHwAcc(acc);
	ctx->hwdec_priv = acc;
	ctx->avctx->hwaccel_context = acc->context();
	return 0;
}

void HwAcc::uninit(lavc_ctx *ctx) {
	if (ctx->hwdec_info && ctx->hwdec_info->vdpau_ctx)
		vo(ctx)->setHwAcc(nullptr);
	delete static_cast<HwAcc*>(ctx->hwdec_priv);
}

mp_image *HwAcc::allocateImage(struct lavc_ctx *ctx, int imgfmt, int width, int height) {
	auto acc = static_cast<HwAcc*>(ctx->hwdec_priv);
	if (imgfmt != acc->d->imgfmt || !acc->isOk())
		return nullptr;
	if (acc->size().width() != width || acc->size().height() != height) {
		if (!acc->fillContext(ctx->avctx))
			return nullptr;
		acc->m_size = QSize(width, height);
	}
	return acc->getSurface();
}

int HwAcc::probe(vd_lavc_hwdec *hwdec, mp_hwdec_info *info, const char *decoder) {
	Q_UNUSED(hwdec);	Q_UNUSED(decoder);
	if (!info || !info->vdpau_ctx)
		return HWDEC_ERR_NO_CTX;
	if (supports((AVCodecID)mp_codec_to_av_codec_id(decoder)))
			return 0;
	return HWDEC_ERR_NO_CODEC;
}

vd_lavc_hwdec create_vaapi_functions() {
	vd_lavc_hwdec hwdec;
	hwdec.type = HWDEC_VAAPI;
	hwdec.allocate_image = HwAcc::allocateImage;
	hwdec.init = HwAcc::init;
	hwdec.uninit = HwAcc::uninit;
	hwdec.probe = HwAcc::probe;
	hwdec.process_image = nullptr;
	static const int formats[] = {IMGFMT_VAAPI, 0};
	hwdec.image_formats = formats;
	return hwdec;
}

vd_lavc_hwdec mp_vd_lavc_vaapi = create_vaapi_functions();

vd_lavc_hwdec create_vdpau_functions() {
	vd_lavc_hwdec hwdec;
	hwdec.type = HWDEC_VDPAU;
	hwdec.allocate_image = HwAcc::allocateImage;
	hwdec.init = HwAcc::init;
	hwdec.uninit = HwAcc::uninit;
	hwdec.probe = HwAcc::probe;
	hwdec.process_image = nullptr;
	static const int formats[] = {IMGFMT_VDPAU, 0};
	hwdec.image_formats = formats;
	return hwdec;
}

vd_lavc_hwdec mp_vd_lavc_vdpau = create_vdpau_functions();

vd_lavc_hwdec create_vda_functions() {
	vd_lavc_hwdec hwdec;
	hwdec.type = HWDEC_VDA;
	hwdec.allocate_image = HwAcc::allocateImage;
	hwdec.init = HwAcc::init;
	hwdec.uninit = HwAcc::uninit;
	hwdec.probe = HwAcc::probe;
	hwdec.process_image = nullptr;
	static const int formats[] = {IMGFMT_VDA, 0};
	hwdec.image_formats = formats;
	return hwdec;
}

vd_lavc_hwdec mp_vd_lavc_vda = create_vda_functions();
