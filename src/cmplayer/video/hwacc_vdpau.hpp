#ifndef HWACC_VDPAU_HPP
#define HWACC_VDPAU_HPP

#include "stdafx.hpp"

#ifdef Q_OS_LINUX

#include "hwacc.hpp"
#include "hwacc_helper.hpp"
#include "misc/tmp.hpp"
#include "misc/log.hpp"
extern "C" {
#include <libavcodec/vdpau.h>
}

auto initialize_vdpau_interop(QOpenGLContext *ctx) -> void;
auto finalize_vdpau_interop(QOpenGLContext *ctx) -> void;

static constexpr auto VDP_DECODER_PROFILE_NONE = (VdpDecoderProfile)-1;

struct VdpauProfile {
    VdpDecoderProfile id = VDP_DECODER_PROFILE_NONE;
    int level = 0, blocks = 0, width = 0, height = 0; // maximum infos
};

#undef Status
template<> struct HwAccX11Trait<IMGFMT_VDPAU> {
    using Profile = VdpauProfile;
    using Status = VdpStatus;
    static constexpr Status success = VDP_STATUS_OK;
    static constexpr const char *name = "VDPAU";
    using SurfaceID = VdpVideoSurface;
    static constexpr SurfaceID invalid = VDP_INVALID_HANDLE;
    static auto error(Status status) -> const char*;
    static auto destroySurface(SurfaceID id) -> void;
    static auto createSurfaces(int w, int h, int format,
                               QVector<SurfaceID> &ids) -> bool;
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

    static auto getErrorString(VdpStatus status) -> char const*
        {return d.getErrorString(status);}
    static auto getInformationString(char const **info) -> VdpStatus
        {return d.getInformationString(info);}
    static auto deviceDestroy() -> VdpStatus {return d.deviceDestroy(d.device);}
    static auto videoSurfaceQueryCaps(VdpChromaType type, VdpBool *ok,
                                      quint32 *maxw, quint32 *maxh) -> VdpStatus
        { return d.videoSurfaceQueryCaps(d.device, type, ok, maxw, maxh); }
    static auto videoSurfaceCreate(VdpChromaType type, quint32 w, quint32 h,
                                   VdpVideoSurface *surface) -> VdpStatus
        { return d.videoSurfaceCreate(d.device, type, w, h, surface); }
    static auto videoSurfaceDestroy(VdpVideoSurface surface) -> VdpStatus
        {return d.videoSurfaceDestroy(surface);}
    static auto decoderQueryCaps(VdpDecoderProfile profile, VdpBool *supported,
                                 quint32 *max_level, quint32 *max_blocks,
                                 quint32 *maxw, quint32 *maxh) -> VdpStatus
    {
        return d.decoderQueryCaps(d.device, profile, supported,
                                  max_level, max_blocks, maxw, maxh);
    }
    static auto decoderCreate(VdpDecoderProfile profile, quint32 w, quint32 h,
                              quint32 refs, VdpDecoder *decoder) -> VdpStatus
        { return d.decoderCreate(d.device, profile, w, h, refs, decoder); }
    static auto decoderRender(VdpDecoder decoder, VdpVideoSurface target,
                              const VdpPictureInfo *picture, quint32 count,
                              const VdpBitstreamBuffer *buffers) -> VdpStatus
        { return d.decoderRender(decoder, target, picture, count, buffers); }
    static auto decoderDestroy(VdpDecoder decoder) -> VdpStatus
        {return d.decoderDestroy(decoder);}
    static auto videoMixerCreate(quint32 feature_count,
                                 const VdpVideoMixerFeature *features,
                                 quint32 parameter_count,
                                 const VdpVideoMixerParameter *parameters,
                                 const void *const *parameter_values,
                                 VdpVideoMixer *mixer) -> VdpStatus
    {
        return d.videoMixerCreate(d.device, feature_count, features,
                                  parameter_count, parameters, parameter_values,
                                  mixer);
    }
    static auto videoMixerDestroy(VdpVideoMixer mixer) -> VdpStatus
        { return d.videoMixerDestroy(mixer); }
    static auto videoMixerRender
        (VdpVideoMixer mixer, VdpOutputSurface bg, const VdpRect *bg_src_rect,
         VdpVideoMixerPictureStructure curt_pic_struct,
         quint32 s_past_count, const VdpVideoSurface *s_past,
         VdpVideoSurface s_current,
         quint32 s_future_count, const VdpVideoSurface *s_future,
         const VdpRect *source_rect,
         VdpOutputSurface dest_surface, const VdpRect *dest_rect,
         const VdpRect *dest_video_rect,
         quint32 layer_count, const VdpLayer *layers) -> VdpStatus
    {
        return d.videoMixerRender(mixer, bg, bg_src_rect, curt_pic_struct,
                                  s_past_count, s_past, s_current,
                                  s_future_count, s_future, source_rect,
                                  dest_surface, dest_rect, dest_video_rect,
                                  layer_count, layers);
    }
    static auto outputSurfaceCreate(VdpRGBAFormat rgba, quint32 w, quint32 h,
                                    VdpOutputSurface * surface) -> VdpStatus
        { return d.outputSurfaceCreate(d.device, rgba, w, h, surface); }
    static auto outputSurfaceDestroy(VdpOutputSurface surface) -> VdpStatus
        { return d.outputSurfaceDestroy(surface); }
    static auto registerOutputSurface(VdpOutputSurface surface, GLenum target,
                                      GLsizei numTexturesNames,
                                      const GLuint *textureNames)
    -> GLvdpauSurfaceNV
    {
        return d.registerOutputSurface(TO_INTEROP(surface), target,
                                       numTexturesNames, textureNames);
    }
    static auto isSurface(GLvdpauSurfaceNV surface) -> GLboolean
        { return d.isSurface(surface); }
    static auto unregisterSurface(GLvdpauSurfaceNV surface) -> void
        { d.unregisterSurface(surface); }
    static auto getSurfaceiv(GLvdpauSurfaceNV surface, GLenum pname,
                             GLsizei bufSize, GLsizei *length,
                             int *values) -> void
        { d.getSurfaceiv(surface, pname, bufSize, length, values); }
    static auto surfaceAccess(GLvdpauSurfaceNV surface, GLenum access) -> void
        { d.surfaceAccess(surface, access); }
    static auto mapSurfaces(GLsizei numSurfaces,
                            const GLvdpauSurfaceNV *surfaces) -> void
        { d.mapSurfaces(numSurfaces, surfaces); }
    static auto unmapSurfaces(GLsizei numSurface,
                              const GLvdpauSurfaceNV *surfaces) -> void
        { d.unmapSurfaces(numSurface, surfaces); }
    static auto isInitialized() -> bool { return d.init; }
    static auto isAvailable() -> bool;
private:
    template<class T, class = typename std::enable_if<!std::is_pointer<T>::value>::type>
    static auto TO_INTEROP(T handle) -> const void *
        { return (const void*)(quintptr)(handle); }
    struct Data : public VdpauStatusChecker {
        VdpDevice device = 0;
        VdpGetProcAddress *proc = nullptr;
        bool init = false;
        VdpGetErrorString *getErrorString = nullptr;
        VdpGetInformationString *getInformationString = nullptr;
        VdpDeviceDestroy *deviceDestroy = nullptr;
        VdpVideoSurfaceQueryCapabilities *videoSurfaceQueryCaps = nullptr;
        VdpVideoSurfaceCreate *videoSurfaceCreate = nullptr;
        VdpVideoSurfaceDestroy *videoSurfaceDestroy = nullptr;
        VdpVideoSurfaceGetBitsYCbCr *videoSurfaceGetBitsYCbCr = nullptr;
        VdpDecoderQueryCapabilities *decoderQueryCaps = nullptr;
        VdpDecoderCreate *decoderCreate = nullptr;
        VdpDecoderRender *decoderRender = nullptr;
        VdpDecoderDestroy *decoderDestroy = nullptr;
        VdpVideoMixerCreate *videoMixerCreate = nullptr;
        VdpVideoMixerDestroy *videoMixerDestroy = nullptr;
        VdpVideoMixerRender *videoMixerRender = nullptr;
        VdpOutputSurfaceCreate *outputSurfaceCreate = nullptr;
        VdpOutputSurfaceDestroy *outputSurfaceDestroy = nullptr;

        GLvdpauSurfaceNV(*registerOutputSurface)(const GLvoid*, GLenum, GLsizei,
                                                 const GLuint*) = nullptr;
        GLboolean (*isSurface) (GLvdpauSurfaceNV) = nullptr;
        void (*unregisterSurface) (GLvdpauSurfaceNV) = nullptr;
        void (*getSurfaceiv) (GLvdpauSurfaceNV, GLenum, GLsizei, GLsizei*,
                              int*) = nullptr;
        void (*surfaceAccess) (GLvdpauSurfaceNV, GLenum) = nullptr;
        void (*mapSurfaces) (GLsizei, const GLvdpauSurfaceNV *) = nullptr;
        void (*unmapSurfaces) (GLsizei, const GLvdpauSurfaceNV *) = nullptr;
        void (*initialize) (const GLvoid *, const GLvoid *) = nullptr;
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
    static auto proc(const QByteArray &name, T &func) -> bool
    {
        func = reinterpret_cast<T>(d.gl->getProcAddress(name));
        return func != nullptr;
    }
};

class HwAccVdpau : public HwAcc, public VdpauStatusChecker {
public:
    HwAccVdpau(AVCodecID cid);
    ~HwAccVdpau();
    virtual auto isOk() const -> bool final { return isSuccess(); }
    virtual auto context() const -> void* final;
    virtual auto getSurface() -> mp_image* final;
    virtual auto type() const -> Type final {return VdpauX11;}
    virtual auto getImage(mp_image *mpi) -> mp_image*;
private:
    auto freeContext() -> void;
    auto fillContext(AVCodecContext *avctx, int w, int h) -> bool final;
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
