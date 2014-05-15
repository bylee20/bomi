#ifndef HWACC_VDPAU_HPP
#define HWACC_VDPAU_HPP

#include "stdafx.hpp"

#ifdef Q_OS_LINUX

#include "hwacc.hpp"
#include "hwacc_helper.hpp"
extern "C" {
#include <libavcodec/vdpau.h>
}
#include "log.hpp"

auto initialize_vdpau_interop(QOpenGLContext *ctx) -> void;
auto finalize_vdpau_interop(QOpenGLContext *ctx) -> void;

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
    static auto destroySurface(SurfaceID id) -> void;
    static auto createSurfaces(int w, int h, int f, QVector<SurfaceID> &ids) -> bool;
};

using VdpauCodec = HwAccX11Codec<IMGFMT_VDPAU>;
using VdpauStatusChecker = HwAccX11StatusChecker<IMGFMT_VDPAU>;
using VdpauSurface = HwAccX11Surface<IMGFMT_VDPAU>;
using VdpauSurfacePool = HwAccX11SurfacePool<IMGFMT_VDPAU>;

class Vdpau {
    DECLARE_LOG_CONTEXT(VDPAU)
public:
    Vdpau() {}
    static auto codec(AVCodecID id) -> const VdpauCodec*
    {
        auto it = d.supports.constFind(id);
        return (it != d.supports.cend()) ? &(*it) : nullptr;
    }

    static auto device() -> VdpDevice { return d.device; }
    static auto initialize() -> void;
    static auto finalize() -> void;
    static auto initializeInterop(QOpenGLContext *ctx) -> void;
    static auto finalizeInterop(QOpenGLContext *ctx) -> void;

    static char const *getErrorString(VdpStatus status) {return d.getErrorString(status);}
    static auto getInformationString(char const **information_string) -> VdpStatus {return d.getInformationString(information_string);}
    static auto deviceDestroy() -> VdpStatus {return d.deviceDestroy(d.device);}
    static auto videoSurfaceQueryCapabilities(VdpChromaType type, VdpBool *supported, uint32_t *max_width, uint32_t *max_height) -> VdpStatus {
        return d.videoSurfaceQueryCapabilities(d.device, type, supported, max_width, max_height);
    }
    static auto videoSurfaceCreate(VdpChromaType type, uint32_t width, uint32_t height, VdpVideoSurface *surface) -> VdpStatus {
        return d.videoSurfaceCreate(d.device, type, width, height, surface);
    }
    static auto videoSurfaceDestroy(VdpVideoSurface surface) -> VdpStatus {return d.videoSurfaceDestroy(surface);}
    static auto videoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat format, void *const *data, uint32_t const *pitches) -> VdpStatus {
        return d.videoSurfaceGetBitsYCbCr(surface, format, data, pitches);
    }
    static auto decoderQueryCapabilities(VdpDecoderProfile profile, VdpBool *supported, uint32_t *max_level, uint32_t *max_macroblocks, uint32_t *max_width, uint32_t *max_height) -> VdpStatus {
        return d.decoderQueryCapabilities(d.device, profile, supported, max_level, max_macroblocks, max_width, max_height);
    }
    static auto decoderCreate(VdpDecoderProfile profile, uint32_t width, uint32_t height, uint32_t max_references, VdpDecoder *decoder) -> VdpStatus {
        return d.decoderCreate(d.device, profile, width, height, max_references, decoder);
    }
    static auto decoderRender(VdpDecoder decoder, VdpVideoSurface target, VdpPictureInfo const *picture_info, uint32_t count, VdpBitstreamBuffer const *buffers) -> VdpStatus {
        return d.decoderRender(decoder, target, picture_info, count, buffers);
    }
    static auto decoderDestroy(VdpDecoder decoder) -> VdpStatus {return d.decoderDestroy(decoder);}
    static VdpStatus videoMixerCreate(uint32_t feature_count, const VdpVideoMixerFeature *features
        , uint32_t parameter_count, const VdpVideoMixerParameter *parameters, const void *const *parameter_values, VdpVideoMixer *mixer) {
        return d.videoMixerCreate(d.device, feature_count, features, parameter_count, parameters, parameter_values, mixer);
    }
    static auto videoMixerDestroy(VdpVideoMixer mixer) -> VdpStatus { return d.videoMixerDestroy(mixer); }
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

    static auto outputSurfaceCreate(VdpRGBAFormat rgba_format, uint32_t width, uint32_t height, VdpOutputSurface * surface) -> VdpStatus {
        return d.outputSurfaceCreate(d.device, rgba_format, width, height, surface);
    }
    static auto outputSurfaceDestroy(VdpOutputSurface surface) -> VdpStatus { return d.outputSurfaceDestroy(surface); }

    static auto registerOutputSurface(VdpOutputSurface surface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames) -> GLvdpauSurfaceNV {
        return d.registerOutputSurface(TO_INTEROP(surface), target, numTextureNames, textureNames);
    }
    static auto isSurface(GLvdpauSurfaceNV surface) -> GLboolean { return d.isSurface(surface); }
    static auto unregisterSurface(GLvdpauSurfaceNV surface) -> void { d.unregisterSurface(surface); }
    static auto getSurfaceiv(GLvdpauSurfaceNV surface, GLenum pname, GLsizei bufSize, GLsizei *length, int *values) -> void {
        d.getSurfaceiv(surface, pname, bufSize, length, values);
    }
    static auto surfaceAccess(GLvdpauSurfaceNV surface, GLenum access) -> void { d.surfaceAccess(surface, access); }
    static auto mapSurfaces(GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces) -> void { d.mapSurfaces(numSurfaces, surfaces); }
    static auto unmapSurfaces(GLsizei numSurface, const GLvdpauSurfaceNV *surfaces) -> void { d.unmapSurfaces(numSurface, surfaces); }
    static auto isInitialized() -> bool { return d.init; }
    static auto isAvailable() -> bool { return d.ok; }
private:
    template<class T, class = typename std::enable_if<!std::is_pointer<T>::value>::type>
    static const void *TO_INTEROP(T handle) { return (const void*)(quintptr)(handle); }
    struct Data : public VdpauStatusChecker {
        VdpDevice device = 0;
        VdpGetProcAddress *proc = nullptr;
        bool init = false, ok = false;
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
    template<class F>
    static auto proc(VdpFuncId id, F &func) -> VdpStatus {
        if (!d.proc || !d.isSuccess())
            return d.status();
        d.isSuccess(d.proc(d.device, id, &reinterpret_cast<void*&>(func)));
        return d.status();
    }
    template<class T>
    static auto proc(const QByteArray &name, T &func) -> bool { return (func = reinterpret_cast<T>(d.gl->getProcAddress(name))) != nullptr; }
};

class HwAccVdpau : public HwAcc, public VdpauStatusChecker {
public:
    HwAccVdpau(AVCodecID cid);
    ~HwAccVdpau();
    virtual auto isOk() const -> bool override { return isSuccess(); }
    virtual void *context() const override;
    virtual mp_image *getSurface() override;
    virtual auto type() const -> Type override {return VdpauX11;}
    virtual mp_image *getImage(mp_image *mpi);
private:
    auto freeContext() -> void;
    auto fillContext(AVCodecContext *avctx, int w, int h) -> bool override;

private:
    struct Data;
    Data *d;
};

class VdpauMixer : public HwAccMixer, public VdpauStatusChecker {
public:
    ~VdpauMixer();
    auto create(const QList<OpenGLTexture2D> &textures) -> bool final;
    auto upload(const mp_image *mpi, bool deint) -> bool final;
    auto directRendering() const -> bool final { return true; }
    auto getAligned(const mp_image *mpi,
                    QVector<QSize> *bytes) -> mp_imgfmt final;
private:
    VdpauMixer(const QSize &size);
    VdpVideoMixer m_mixer = VDP_INVALID_HANDLE;
    VdpChromaType m_chroma = VDP_CHROMA_TYPE_420;
    VdpOutputSurface m_surface = VDP_INVALID_HANDLE;
    GLvdpauSurfaceNV m_glSurface = GL_NONE;
    friend class HwAcc;
};

#endif

#endif // HWACC_VDPAU_HPP
