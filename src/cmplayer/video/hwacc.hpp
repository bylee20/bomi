#ifndef HWACC_HPP
#define HWACC_HPP

#include "stdafx.hpp"
#include <array>
extern "C" {
#include <libavcodec/avcodec.h>
#include <video/img_format.h>
#undef bswap_16
#undef bswap_32
#include <video/decode/lavc.h>
}
#include "enums.hpp"

struct lavc_ctx;                        struct mp_image;
struct vd_lavc_hwdec;                   struct mp_hwdec_info;
class VideoOutput;                      class OpenGLTexture2D;

class HwAccMixer {
public:
    HwAccMixer(const QSize &size): m_size(size) { }
    virtual ~HwAccMixer() {}
    virtual auto getAligned(const mp_image *mpi,
                            QVector<QSize> *bytes) -> mp_imgfmt = 0;
    virtual auto create(const QList<OpenGLTexture2D> &textures) -> bool = 0;
    virtual auto directRendering() const -> bool = 0;
    virtual auto upload(const mp_image *mpi, bool deint) -> bool = 0;
    auto size() const -> const QSize& { return m_size; }
    auto width() const -> int { return m_size.width(); }
    auto height() const -> int { return m_size.height(); }
private:
    QSize m_size;
};

class HwAcc {
public:
    enum Type {None, VaApiGLX, VdpauX11, Vda};
    virtual ~HwAcc();
    static auto initialize() -> void;
    static auto finalize() -> void;
    static auto availableBackends() -> QList<Type>;
    static auto fullCodecList() -> QList<AVCodecID>;
    static auto fullDeintList() -> QList<DeintMethod>;
    static auto supports(Type backend, AVCodecID codec) -> bool;
    static auto supports(DeintMethod method) -> bool;
    static auto backend(const QString &name) -> Type;
    static auto backendDescription(Type type) -> QString;
    static auto backendName(Type type) -> QString;
    static auto codecName(int id) -> const char*;
    static auto codecId(const char *name) -> AVCodecID;
    static auto createMixer(mp_imgfmt imgfmt, const QSize &size) -> HwAccMixer*;
    auto imgfmt() const -> int;
    virtual auto getImage(mp_image *mpi) -> mp_image* = 0;
    virtual auto type() const -> Type = 0;
protected:
    HwAcc(AVCodecID codec);
    virtual auto isOk() const -> bool = 0;
    virtual auto getSurface() -> mp_image* = 0;
    virtual auto context() const -> void* = 0;
    virtual auto fillContext(AVCodecContext *avctx, int w, int h) -> bool = 0;
    auto codec() const -> AVCodecID {return m_codec;}
    auto size() const -> const QSize& {return m_size;}
private:
    static auto vo(lavc_ctx *ctx) -> VideoOutput*;
    static auto probe(vd_lavc_hwdec *hwdec,
                      mp_hwdec_info *info, const char *decoder) -> int;
    static auto init(lavc_ctx *ctx) -> int;
    static auto uninit(lavc_ctx *ctx) -> void;
    static auto allocateImage(lavc_ctx *ctx, int imgfmt,
                              int w, int h) -> mp_image*;
    static auto initDecoder(lavc_ctx *ctx, int fmt, int w, int h) -> int;
    friend class PlayEngine;
    friend auto create_vaapi_functions() -> vd_lavc_hwdec;
    friend auto create_vdpau_functions() -> vd_lavc_hwdec;
    friend auto create_vda_functions() -> vd_lavc_hwdec;
    friend auto create_lavc_hwdec(hwdec_type, mp_imgfmt) -> vd_lavc_hwdec;
    struct Data;
    Data *d;
    AVCodecID m_codec = AV_CODEC_ID_NONE;
    QSize m_size = {0,0};
};

#endif // HWACC_HPP
