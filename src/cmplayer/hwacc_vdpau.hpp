#ifndef HWACC_VDPAU_HPP
#define HWACC_VDPAU_HPP

#if 0

#include "hwacc.hpp"
extern "C" {
#include <libavcodec/vdpau.h>
}
// TODO: GL_NV_vdpau_interop implementation


struct VdpauProfile {
	VdpDecoderProfile id = (VdpDecoderProfile)-1;
	int level = 0, blocks = 0, width = 0, height = 0; // maximum infos
};

typedef HwAccCodec<VdpauProfile> VdpauCodec;

class Vdpau {
public:
	Vdpau() {}
	static const VdpauCodec *codec(AVCodecID id) {
		auto it = d.supports.constFind(id); return (it != d.supports.cend()) ? &(*it) : nullptr;
	}

	static VdpDevice device() {return d.device;}
	static void initialize();
	static void finalize();

	static char const *getErrorString(VdpStatus status) {return d.getErrorString(status);}
	static VdpStatus getInformationString(char const **information_string) {return d.getInformationString(information_string);}
	static VdpStatus deviceDestroy() {return d.deviceDestroy(d.device);}
	static VdpStatus videoSurfaceQueryCapabilities(VdpChromaType type, VdpBool *supported, uint32_t *max_width, uint32_t *max_height) {
		return d.videoSurfaceQueryCapabilities(d.device, type, supported, max_width, max_height);
	}
	static VdpStatus videoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpChromaType type, VdpYCbCrFormat format, VdpBool *supported) {
		return d.videoSurfaceQueryGetPutBitsYCbCrCapabilities(d.device, type, format, supported);
	}
	static VdpStatus videoSurfaceCreate(VdpChromaType type, uint32_t width, uint32_t height, VdpVideoSurface *surface) {
		return d.videoSurfaceCreate(d.device, type, width, height, surface);
	}
	static VdpStatus videoSurfaceDestroy(VdpVideoSurface surface) {return d.videoSurfaceDestroy(surface);}
	static VdpStatus videoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat format, void *const *data, uint32_t const *pitches) {
		return d.videoSurfaceGetBitsYCbCr(surface, format, data, pitches);
	}
	static VdpStatus decoderQueryCapabilities(VdpDecoderProfile profile, VdpBool *supported, uint32_t *max_level, uint32_t *max_macroblocks, uint32_t *max_width, uint32_t *max_height) {
		return d.decoderQueryCapabilities(d.device, profile, supported, max_level, max_macroblocks, max_width, max_height);
	}
	static VdpStatus decoderCreate(VdpDecoderProfile profile, uint32_t width, uint32_t height, uint32_t max_references, VdpDecoder *decoder) {
		return d.decoderCreate(d.device, profile, width, height, max_references, decoder);
	}
	static VdpStatus decoderRender(VdpDecoder decoder, VdpVideoSurface target, VdpPictureInfo const *picture_info, uint32_t count, VdpBitstreamBuffer const *buffers) {
		return d.decoderRender(decoder, target, picture_info, count, buffers);
	}
	static VdpStatus decoderDestroy(VdpDecoder decoder) {return d.decoderDestroy(decoder);}
private:
	struct Data {
		VdpDevice device = 0;
		VdpGetProcAddress *proc = nullptr;
		bool init = false;
		VdpStatus status = VDP_STATUS_ERROR;
		VdpGetErrorString *getErrorString = nullptr;
		VdpGetInformationString *getInformationString = nullptr;
		VdpDeviceDestroy *deviceDestroy = nullptr;
		VdpVideoSurfaceQueryCapabilities *videoSurfaceQueryCapabilities = nullptr;
		VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities *videoSurfaceQueryGetPutBitsYCbCrCapabilities = nullptr;
		VdpVideoSurfaceCreate *videoSurfaceCreate = nullptr;
		VdpVideoSurfaceDestroy *videoSurfaceDestroy = nullptr;
		VdpVideoSurfaceGetBitsYCbCr *videoSurfaceGetBitsYCbCr = nullptr;
		VdpDecoderQueryCapabilities *decoderQueryCapabilities = nullptr;
		VdpDecoderCreate *decoderCreate = nullptr;
		VdpDecoderRender *decoderRender = nullptr;
		VdpDecoderDestroy *decoderDestroy = nullptr;
		QMap<AVCodecID, VdpauCodec> supports;
	};
	static Data d;
	template<typename F>
	static VdpStatus proc(VdpFuncId id, F &func) {
		if (!d.proc || d.status != VDP_STATUS_OK)
			return d.status;
		return (d.status = d.proc(d.device, id, &reinterpret_cast<void*&>(func)));
	}
};

class HwAccVdpau : public HwAcc {
public:
	HwAccVdpau(AVCodecID cid);
	~HwAccVdpau();
	virtual bool isOk() const override;
	virtual bool check(AVCodecContext *avctx) override;
	virtual mp_image *getSurface() override;
	virtual bool isAvailable(AVCodecID codec) const override;
	virtual void *context() const override;
	virtual mp_image *getImage(mp_image *mpi) override;
	virtual Type type() const override {return VdpauX11;}
	bool fillContext(AVCodecContext *avctx);
	void freeContext();
private:
	struct Data;
	Data *d;
};

#endif

#endif // HWACC_VDPAU_HPP
