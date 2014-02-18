#ifndef HWACC_VDPAU_HPP
#define HWACC_VDPAU_HPP

#include "hwacc.hpp"
#include "hwacc_helper.hpp"
extern "C" {
#include <libavcodec/vdpau.h>
}
#include "log.hpp"

void initialize_vdpau_interop(QOpenGLContext *ctx);
void finalize_vdpau_interop(QOpenGLContext *ctx);

struct VdpauProfile {
	VdpDecoderProfile id = (VdpDecoderProfile)-1;
	int level = 0, blocks = 0, width = 0, height = 0; // maximum infos
};

#undef Status
template<> struct HwAccX11Trait<IMGFMT_VDPAU> {
	using Profile = VdpauProfile;
	using Status = VdpStatus;
	static constexpr Status success = VDP_STATUS_OK;
	static constexpr const char *name = "VDPAU";
	static const char *error(Status status);
	using SurfaceID = VdpVideoSurface;
	static constexpr SurfaceID invalid = VDP_INVALID_HANDLE;
	static void destroySurface(SurfaceID id);
	static bool createSurfaces(int w, int h, int f, QVector<SurfaceID> &ids);
};

using VdpauCodec = HwAccX11Codec<IMGFMT_VDPAU>;
using VdpauStatusChecker = HwAccX11StatusChecker<IMGFMT_VDPAU>;
using VdpauSurface = HwAccX11Surface<IMGFMT_VDPAU>;
using VdpauSurfacePool = HwAccX11SurfacePool<IMGFMT_VDPAU>;

class Vdpau {
	DECLARE_LOG_CONTEXT(VDPAU)
public:
	Vdpau() {}
	static const VdpauCodec *codec(AVCodecID id) {
		auto it = d.supports.constFind(id); return (it != d.supports.cend()) ? &(*it) : nullptr;
	}

	static VdpDevice device() {return d.device;}
	static void initialize();
	static void finalize();
	static void initializeInterop(QOpenGLContext *ctx);
	static void finalizeInterop(QOpenGLContext *ctx);

	static char const *getErrorString(VdpStatus status) {return d.getErrorString(status);}
	static VdpStatus getInformationString(char const **information_string) {return d.getInformationString(information_string);}
	static VdpStatus deviceDestroy() {return d.deviceDestroy(d.device);}
	static VdpStatus videoSurfaceQueryCapabilities(VdpChromaType type, VdpBool *supported, uint32_t *max_width, uint32_t *max_height) {
		return d.videoSurfaceQueryCapabilities(d.device, type, supported, max_width, max_height);
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
	static VdpStatus videoMixerCreate(uint32_t feature_count, const VdpVideoMixerFeature *features
		, uint32_t parameter_count, const VdpVideoMixerParameter *parameters, const void *const *parameter_values, VdpVideoMixer *mixer) {
		return d.videoMixerCreate(d.device, feature_count, features, parameter_count, parameters, parameter_values, mixer);
	}
	static VdpStatus videoMixerDestroy(VdpVideoMixer mixer) { return d.videoMixerDestroy(mixer); }
	static VdpStatus videoMixerRender(VdpVideoMixer mixer, VdpOutputSurface background_surface, const VdpRect *background_source_rect
		, VdpVideoMixerPictureStructure current_picture_structure, uint32_t video_surface_past_count, const VdpVideoSurface *video_surface_past
		, VdpVideoSurface video_surface_current,uint32_t video_surface_future_count, const VdpVideoSurface *video_surface_future
		, const VdpRect *video_source_rect, VdpOutputSurface destination_surface
		, const VdpRect *destination_rect, const VdpRect *destination_video_rect, uint32_t layer_count, const VdpLayer *layers) {
		return d.videoMixerRender(mixer, background_surface, background_source_rect
								  , current_picture_structure, video_surface_past_count, video_surface_past
								  , video_surface_current, video_surface_future_count, video_surface_future
								  , video_source_rect, destination_surface
								  , destination_rect, destination_video_rect, layer_count, layers);
	}

	static VdpStatus outputSurfaceCreate(VdpRGBAFormat rgba_format, uint32_t width, uint32_t height, VdpOutputSurface * surface) {
		return d.outputSurfaceCreate(d.device, rgba_format, width, height, surface);
	}
	static VdpStatus outputSurfaceDestroy(VdpOutputSurface surface) { return d.outputSurfaceDestroy(surface); }

	static GLvdpauSurfaceNV registerOutputSurface(VdpOutputSurface surface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames) {
		return d.registerOutputSurface(TO_INTEROP(surface), target, numTextureNames, textureNames);
	}
	static GLboolean isSurface(GLvdpauSurfaceNV surface) { return d.isSurface(surface); }
	static void unregisterSurface(GLvdpauSurfaceNV surface) { d.unregisterSurface(surface); }
	static void getSurfaceiv(GLvdpauSurfaceNV surface, GLenum pname, GLsizei bufSize, GLsizei *length, int *values) {
		d.getSurfaceiv(surface, pname, bufSize, length, values);
	}
	static void surfaceAccess(GLvdpauSurfaceNV surface, GLenum access) { d.surfaceAccess(surface, access); }
	static void mapSurfaces(GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces) { d.mapSurfaces(numSurfaces, surfaces); }
	static void unmapSurfaces(GLsizei numSurface, const GLvdpauSurfaceNV *surfaces) { d.unmapSurfaces(numSurface, surfaces); }
	static bool isInitialized() { return d.init; }
private:
	template<class T, class = typename std::enable_if<!std::is_pointer<T>::value>::type>
	static const void *TO_INTEROP(T handle) { return (const void*)(quintptr)(handle); }
	struct Data : public VdpauStatusChecker {
		VdpDevice device = 0;
		VdpGetProcAddress *proc = nullptr;
		bool init = false;
		VdpGetErrorString *getErrorString = nullptr;
		VdpGetInformationString *getInformationString = nullptr;
		VdpDeviceDestroy *deviceDestroy = nullptr;
		VdpVideoSurfaceQueryCapabilities *videoSurfaceQueryCapabilities = nullptr;
		VdpVideoSurfaceCreate *videoSurfaceCreate = nullptr;
		VdpVideoSurfaceDestroy *videoSurfaceDestroy = nullptr;
		VdpVideoSurfaceGetBitsYCbCr *videoSurfaceGetBitsYCbCr = nullptr;
		VdpDecoderQueryCapabilities *decoderQueryCapabilities = nullptr;
		VdpDecoderCreate *decoderCreate = nullptr;
		VdpDecoderRender *decoderRender = nullptr;
		VdpDecoderDestroy *decoderDestroy = nullptr;
		VdpVideoMixerCreate *videoMixerCreate = nullptr;
		VdpVideoMixerDestroy *videoMixerDestroy = nullptr;
		VdpVideoMixerRender *videoMixerRender = nullptr;
		VdpOutputSurfaceCreate *outputSurfaceCreate = nullptr;
		VdpOutputSurfaceDestroy *outputSurfaceDestroy = nullptr;

		GLvdpauSurfaceNV (*registerOutputSurface) (const GLvoid *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames) = nullptr;
		GLboolean (*isSurface) (GLvdpauSurfaceNV surface) = nullptr;
		void (*unregisterSurface) (GLvdpauSurfaceNV surface) = nullptr;
		void (*getSurfaceiv) (GLvdpauSurfaceNV surface, GLenum pname, GLsizei bufSize, GLsizei *length, int *values) = nullptr;
		void (*surfaceAccess) (GLvdpauSurfaceNV surface, GLenum access) = nullptr;
		void (*mapSurfaces) (GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces) = nullptr;
		void (*unmapSurfaces) (GLsizei numSurface, const GLvdpauSurfaceNV *surfaces) = nullptr;
		void (*initialize) (const GLvoid *vdpDevice, const GLvoid *getProcAddress) = nullptr;
		void (*finalize) (void) = nullptr;

		QOpenGLContext *gl = nullptr;
		QMap<AVCodecID, VdpauCodec> supports;
	};
	static Data d;
	template<typename F>
	static VdpStatus proc(VdpFuncId id, F &func) {
		if (!d.proc || !d.isSuccess())
			return d.status();
		d.isSuccess(d.proc(d.device, id, &reinterpret_cast<void*&>(func)));
		return d.status();
	}
	template<typename T>
	static bool proc(const QByteArray &name, T &func) { return (func = reinterpret_cast<T>(d.gl->getProcAddress(name))) != nullptr; }
};

class HwAccVdpau : public HwAcc, public VdpauStatusChecker {
public:
	HwAccVdpau(AVCodecID cid);
	~HwAccVdpau();
	virtual bool isOk() const override { return isSuccess(); }
	virtual void *context() const override;
	virtual mp_image *getSurface() override;
	virtual Type type() const override {return VdpauX11;}
	virtual mp_image *getImage(mp_image *mpi);
private:
	void freeContext();
	bool fillContext(AVCodecContext *avctx) override;

private:
	struct Data;
	Data *d;
};

class VdpauMixer : public HwAccMixer, public VdpauStatusChecker {
public:
	VdpauMixer(const OpenGLTexture &texture, const VideoFormat &format);
	~VdpauMixer();
	bool upload(VideoFrame &frame, bool deint) override;
private:
	quint32 m_width = 0, m_height = 0;
	VdpVideoMixer m_mixer = VDP_INVALID_HANDLE;
	VdpChromaType m_chroma = VDP_CHROMA_TYPE_420;
	VdpOutputSurface m_surface = VDP_INVALID_HANDLE;
	GLvdpauSurfaceNV m_glSurface = GL_NONE;
};

#endif // HWACC_VDPAU_HPP
