//#ifndef VIDEOFORMAT_HPP
//#define VIDEOFORMAT_HPP

//extern "C" {
//#include <video/mp_image.h>
//}

//class VideoFormat {
//public:
//    using Type = mp_imgfmt;
//    VideoFormat(const mp_image_params &params, const mp_imgfmt_desc &desc,
//                bool inverted)
//        : d(new Data(params, desc, inverted)) { }
//    VideoFormat(): d(new Data) { }
//    auto operator == (const VideoFormat &rhs) const -> bool
//        { return isSame(d->params, rhs.d->params)
//                 && d->inverted == rhs.d->inverted; }
//    auto operator != (const VideoFormat &rhs) const -> bool
//        { return !operator == (rhs); }
//    auto params() const -> const mp_image_params& { return d->params; }
//    auto isEmpty() const -> bool;
//    auto width() const -> int { return d->params.w; }
//    auto height() const -> int { return d->params.h; }
//    auto size() const -> QSize { return QSize(d->params.w, d->params.h); }
//    auto displaySize() const -> QSize
//        { return QSize(d->params.d_w, d->params.d_h); }
//    auto bitrate(double fps) const -> double
//        { return fps*d->params.w*d->params.h*d->bpp;}
//    auto name() const -> QByteArray { return name(d->params.imgfmt); }
//    auto colorspace() const -> mp_csp { return d->params.colorspace; }
//    auto range() const -> mp_csp_levels { return d->params.colorlevels; }
//    auto chroma() const -> mp_chroma_location
//        { return d->params.chroma_location; }
//    auto isInverted() const -> bool { return d->inverted; }
//    static auto name(Type type) -> QByteArray;
//    static auto bpp(const mp_imgfmt_desc &desc) -> int;
//private:
//    struct Data : public QSharedData {
//        Data() { }
//        Data(const mp_image_params &params, const mp_imgfmt_desc &desc,
//             bool inverted);
//        int bpp = 0;
//        bool inverted = false;
//        mp_image_params params {
//            IMGFMT_NONE, 0, 0, 0, 0, // w, h, d_w, d_h
//            MP_CSP_BT_709,    MP_CSP_LEVELS_PC,
//            MP_CSP_PRIM_BT_709, MP_CHROMA_CENTER, MP_CSP_LEVELS_PC, 0,
//            MP_STEREO3D_MONO, MP_STEREO3D_MONO
//        };
//    };
//    static auto isSame(const mp_image_params &p1,
//                       const mp_image_params &p2) -> bool {
//        return p1.imgfmt == p2.imgfmt
//               && p1.w == p2.w && p1.h == p2.h
//               && p1.d_w == p2.d_w && p1.d_h == p2.d_h
//               && p1.colorspace == p2.colorspace
//               && p1.colorlevels == p2.colorlevels
//               && p1.chroma_location == p2.chroma_location;
//    }
//    QSharedDataPointer<Data> d;
//};

//inline auto VideoFormat::isEmpty() const -> bool
//{
//    return d->params.w <= 0 || d->params.h <= 0
//           || d->params.imgfmt == IMGFMT_NONE;
//}

//inline auto VideoFormat::name(Type type)  -> QByteArray
//{
//    char buf[16] = { 0 };
//    return mp_imgfmt_to_name_buf(buf, sizeof(buf), type);
//}

//#endif // VIDEOFORMAT_HPP
