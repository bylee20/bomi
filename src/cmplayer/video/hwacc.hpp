#ifndef HWACC_HPP
#define HWACC_HPP

extern "C" {
#include <libavcodec/avcodec.h>
#include <video/img_format.h>
}
#include <QObject>

struct lavc_ctx;
struct vd_lavc_hwdec;                   struct mp_hwdec_info;
class VideoOutput;                      class OpenGLTexture2D;
class VideoTexture;                     class MpImage;
enum class DeintMethod;

class HwAcc {
public:
    enum Type {None, VaApiGLX, VdpauX11};
//    virtual ~HwAcc();
    static auto isAvailable() -> bool;
    static auto initialize() -> void;
    static auto finalize() -> void;
    static auto fullCodecList() -> QStringList;
    static auto fullDeintList() -> QList<DeintMethod>;
    static auto codecDescription(const QString &codec) -> QString;
    static auto supports(const QString &codecs) -> bool;
    static auto supports(DeintMethod method) -> bool;
    static auto type(const QString &name) -> Type;
    static auto type() -> Type;
    static auto description() -> QString;
    static auto name(Type type) -> QString;
    static auto name() -> QString;
//    static auto codecName(int id) -> const char*;
//    static auto codecId(const char *type) -> AVCodecID;
//    auto imgfmt() const -> int;
//    virtual auto getImage(const MpImage &mpi) -> MpImage = 0;
//    virtual auto type() const -> Type = 0;
//protected:
//    HwAcc(AVCodecID codec);
//    virtual auto isOk() const -> bool = 0;
//    virtual auto getSurface() -> mp_image* = 0;
//    virtual auto context() const -> void* = 0;
//    virtual auto fillContext(AVCodecContext *avctx, int w, int h) -> bool = 0;
//    auto codec() const -> AVCodecID {return m_codec;}
//    auto size() const -> const QSize& {return m_size;}
//private:
//    static auto vo(lavc_ctx *ctx) -> VideoOutput*;
//    static auto probe(vd_lavc_hwdec *hwdec,
//                      mp_hwdec_info *info, const char *decoder) -> int;
//    static auto init(lavc_ctx *ctx) -> int;
//    static auto uninit(lavc_ctx *ctx) -> void;
//    static auto allocateImage(lavc_ctx *ctx, int imgfmt,
//                              int w, int h) -> mp_image*;
//    static auto initDecoder(lavc_ctx *ctx, int fmt, int w, int h) -> int;
//    friend class PlayEngine;
//    friend auto create_vaapi_functions() -> vd_lavc_hwdec;
//    friend auto create_vdpau_functions() -> vd_lavc_hwdec;
//    friend auto create_vda_functions() -> vd_lavc_hwdec;
//    friend auto create_lavc_hwdec(hwdec_type, mp_imgfmt) -> vd_lavc_hwdec;
//    struct Data;
//    Data *d;
//    AVCodecID m_codec = AV_CODEC_ID_NONE;
//    QSize m_size = {0,0};
};

Q_DECLARE_METATYPE(HwAcc::Type);

#endif // HWACC_HPP
