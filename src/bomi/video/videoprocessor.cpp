#include "videoprocessor.hpp"
#include "videofilter.hpp"
#include "mpimage.hpp"
#include "softwaredeinterlacer.hpp"
#include "motioninterpolator.hpp"
#include "motionintrploption.hpp"
#include "deintoption.hpp"
#include "player/mpv_helper.hpp"
#include "opengl/opengloffscreencontext.hpp"
#include "os/os.hpp"
#include "enum/colorrange.hpp"
#include "enum/colorspace.hpp"
extern "C" {
#include <video/filter/vf.h>
#include <video/hwdec.h>
#include <video/mp_image_pool.h>
extern vf_info vf_info_noformat;
}

struct bomi_vf_priv {
    VideoProcessor *vp;
    char *address, *swdec_deint, *hwdec_deint;
    int interpolate, color_range, color_space;
};

static auto priv(vf_instance *vf) -> VideoProcessor*
{
    return reinterpret_cast<bomi_vf_priv*>(vf->priv)->vp;
}

auto create_vf_info() -> vf_info
{
#define MPV_OPTION_BASE bomi_vf_priv
    static m_option options[] = {
        MPV_OPTION(address),
        MPV_OPTION(swdec_deint),
        MPV_OPTION(hwdec_deint),
        MPV_OPTION(interpolate),
        MPV_OPTION(color_space),
        MPV_OPTION(color_range),
        mpv::null_option
    };

    static vf_info info;
    info.description = "bomi video filter";
    info.name = "noformat";
    info.open = VideoProcessor::open;
    info.options = options;
    info.priv_size = sizeof(bomi_vf_priv);

    return info;
}

class HwDecTool {
public:
    HwDecTool(mp_hwdec_ctx *hwctx)
    {
        m_ctx = hwctx;
        m_pool = mp_image_pool_new(2);
    }
    virtual ~HwDecTool() { talloc_free(m_pool); }
    virtual auto download(const MpImage &src) -> MpImage
    {
        auto img = OS::hwAcc()->download(m_ctx, src.data(), m_pool);
        return img ? MpImage::wrap(img) : MpImage();
    }
protected:
    mp_hwdec_ctx *m_ctx = nullptr;
    mp_image_pool *m_pool = nullptr;
};

vf_info vf_info_noformat = create_vf_info();

struct VideoProcessor::Data {
    VideoProcessor *p = nullptr;
    vf_instance *vf = nullptr;
    DeintOption deint_swdec, deint_hwdec;
    SoftwareDeinterlacer deinterlacer;
    PassthroughVideoFilter passthrough;
    VideoFilter *filter = nullptr;
    MotionInterpolator interpolator;
    MotionIntrplOption intrplOption;
    mp_image_params params;
    ColorSpace spaceIn = ColorSpace::Auto, spaceOut = ColorSpace::Auto, spaceOpt = ColorSpace::Auto;
    ColorRange rangeIn = ColorRange::Auto, rangeOut = ColorRange::Auto, rangeOpt = ColorRange::Auto;
    mp_csp mp_csp_out = MP_CSP_AUTO;
    mp_csp_levels mp_lv_out = MP_CSP_LEVELS_AUTO;
    int hwdecType = -10;
    bool deint = false, inter_i = false, inter_o = false, interpolate = false;
    bool hwacc = false;
    HwDecTool *hwdec = nullptr;
    mp_image_pool *pool = nullptr;

    QMutex mutex; // must be locked
    double ptsSkipStart = MP_NOPTS_VALUE, ptsLastSkip = MP_NOPTS_VALUE;
    bool skip = false;

    auto reset() -> void
    {
        deinterlacer.clear();
        passthrough.clear();
        interpolator.clear();
        filter = nullptr;
    }
    auto updateDeint() -> void
    {
        DeintOption opt;
        if (deint)
            opt = hwdecType > 0 ? deint_hwdec : deint_swdec;
        opt.processor = hwacc ? Processor::GPU : Processor::CPU;
        deinterlacer.setOption(opt);
        reset();
        emit p->deintMethodChanged(opt.method);
    }
};

VideoProcessor::VideoProcessor()
    : d(new Data)
{
    d->p = this;
    d->pool = mp_image_pool_new(1);
}

VideoProcessor::~VideoProcessor()
{
    talloc_free(d->pool);
    delete d->hwdec;
    delete d;
}

auto VideoProcessor::setMotionIntrplOption(const MotionIntrplOption &option) -> void
{
    d->intrplOption = option;
}

auto VideoProcessor::open(vf_instance *vf) -> int
{
    auto p = reinterpret_cast<bomi_vf_priv*>(vf->priv);
    p->vp = address_cast<VideoProcessor*>(p->address);
    p->vp->d->vf = vf;
    auto d = p->vp->d;
    if (p->swdec_deint)
        d->deint_swdec = DeintOption::fromString(_L(p->swdec_deint));
    if (p->hwdec_deint)
        d->deint_hwdec = DeintOption::fromString(_L(p->hwdec_deint));
    d->spaceOpt = (ColorSpace)p->color_space;
    d->rangeOpt = (ColorRange)p->color_range;
    d->updateDeint();
    memset(&d->params, 0, sizeof(d->params));
    vf->reconfig = [] (vf_instance *vf, mp_image_params *in,
                       mp_image_params *out) -> int
        { return priv(vf)->reconfig(in, out); };
    vf->filter_ext = [] (vf_instance *vf, mp_image *in) -> int
        { return priv(vf)->filterIn(in); };
    vf->filter_out = [] (vf_instance *vf) -> int
        { return priv(vf)->filterOut(); };
    vf->needs_input = [] (vf_instance *vf) -> bool
        { return priv(vf)->needsInput(); };
    vf->query_format = queryFormat;
    vf->uninit = [] (vf_instance *vf) -> void { return priv(vf)->uninit(); };
    vf->control = [] (vf_instance *vf, int request, void *data) -> int
        { return priv(vf)->control(request, data); };

    _Delete(d->hwdec);
    hwdec_request_api(vf->hwdec, OS::hwAcc()->name().toLatin1());
    if (vf->hwdec && vf->hwdec->hwctx)
        d->hwdec = new HwDecTool(vf->hwdec->hwctx);
    mp_image_pool_clear(d->pool);
    p->vp->stopSkipping();
    return true;
}

auto VideoProcessor::isInputInterlaced() const -> bool
{
    return d->inter_i;
}

auto VideoProcessor::isOutputInterlaced() const -> bool
{
    return d->inter_o;
}

#define conv_csp(in, def) [&] () { \
    switch (in) { \
    CSP_PAIR(MP_CSP_AUTO, ColorSpace::Auto); \
    CSP_PAIR(MP_CSP_BT_601, ColorSpace::BT601); \
    CSP_PAIR(MP_CSP_BT_709, ColorSpace::BT709); \
    CSP_PAIR(MP_CSP_SMPTE_240M, ColorSpace::SMPTE240M); \
    CSP_PAIR(MP_CSP_BT_2020_NC, ColorSpace::BT2020NCL); \
    CSP_PAIR(MP_CSP_BT_2020_C, ColorSpace::BT2020CL); \
    CSP_PAIR(MP_CSP_XYZ, ColorSpace::XYZ); \
    CSP_PAIR(MP_CSP_YCGCO, ColorSpace::YCgCo); \
    default: return def; }}()
#define CSP_PAIR(m, b) case m: return b;
static auto mp2enum(mp_csp csp) -> ColorSpace
    { return conv_csp(csp, ColorSpace::Auto);}
#undef CSP_PAIR
#define CSP_PAIR(m, b) case b: return m;
static auto enum2mp(ColorSpace csp) -> mp_csp
    { return conv_csp(csp, MP_CSP_AUTO); }
#undef CSP_PAIR
#undef conv_csp

#define conv_lv(in, def) [&] () { \
    switch (in) {\
    LV_PAIR(MP_CSP_LEVELS_AUTO, ColorRange::Auto); \
    LV_PAIR(MP_CSP_LEVELS_TV, ColorRange::Limited); \
    LV_PAIR(MP_CSP_LEVELS_PC, ColorRange::Full); \
    default: return def; }}()
#define LV_PAIR(m, b) case m: return b;
static auto mp2enum(mp_csp_levels lv) -> ColorRange
    { return conv_lv(lv, ColorRange::Auto); }
#undef LV_PAIR
#define LV_PAIR(m, b) case b: return m;
static auto enum2mp(ColorRange lv) -> mp_csp_levels
    { return conv_lv(lv, MP_CSP_LEVELS_AUTO); }
#undef LV_PAIR
#undef conv_lv


auto VideoProcessor::reconfig(mp_image_params *in, mp_image_params *out) -> int
{
    d->params = *in;
    *out = *in;

    d->mp_csp_out = MP_CSP_AUTO;
    d->mp_lv_out = MP_CSP_LEVELS_AUTO;
    if (d->spaceOpt != ColorSpace::Auto)
        d->mp_csp_out = out->colorspace = enum2mp(d->spaceOpt);
    if (d->rangeOpt != ColorRange::Auto)
        d->mp_lv_out = out->colorlevels = enum2mp(d->rangeOpt);

    if (_Change(d->spaceIn, mp2enum(in->colorspace)))
        emit inputColorSpaceChanged(d->spaceIn);
    if (_Change(d->rangeIn, mp2enum(in->colorlevels)))
        emit inputColorRangeChanged(d->rangeIn);
    if (_Change(d->spaceOut, mp2enum(out->colorspace)))
        emit outputColorSpaceChanged(d->spaceOut);
    if (_Change(d->rangeOut, mp2enum(out->colorlevels)))
        emit outputColorRangeChanged(d->rangeOut);

    d->interpolator.setTargetFps(d->intrplOption.fps());
    d->reset();
    d->hwdecType = -10;
    return 0;
}

auto VideoProcessor::skipToNextBlackFrame() -> void
{
    d->mutex.lock();
    if (_Change(d->skip, true))
        emit skippingChanged(d->skip);
    d->mutex.unlock();
}

auto VideoProcessor::stopSkipping() -> void
{
    d->mutex.lock();
    if (_Change(d->skip, false))
        emit skippingChanged(d->skip);
    d->ptsLastSkip = d->ptsSkipStart = MP_NOPTS_VALUE;
    d->mutex.unlock();
}

auto VideoProcessor::isSkipping() const -> bool
{
    return d->skip;
}

template<class F>
static auto avgLuma(const mp_image *mpi, F &&addLine) -> double
{
    const uchar *const data = mpi->planes[0];
    double avg = 0;
    for (int y = 0; y < mpi->h; ++y)
         addLine(avg, data + y * mpi->stride[0], mpi->w);
    const int bits = mpi->fmt.plane_bits;
    avg /= mpi->h * mpi->w;
    avg /= (1 << bits) - 1;
    if (mpi->params.colorlevels == MP_CSP_LEVELS_TV)
        avg = (avg - 16.0/255)*255.0/(235.0 - 16.0);
    return avg;
}

template<class T>
static auto lumaYCbCrPlanar(const mp_image *mpi) -> double
{
    return avgLuma(mpi, [] (double &sum, const uchar *data, int w) {
        const T *p = (const T*)data;
        for (int x = 0; x < w; ++x)
            sum += *p++;
    });
}

static auto luminance(const mp_image *mpi) -> double
{
    switch (mpi->imgfmt) {
    case IMGFMT_420P:   case IMGFMT_NV12:   case IMGFMT_NV21:
    case IMGFMT_444P:   case IMGFMT_422P:   case IMGFMT_440P:
    case IMGFMT_411P:   case IMGFMT_410P:   case IMGFMT_Y8:
    case IMGFMT_444AP:  case IMGFMT_422AP:  case IMGFMT_420AP:
        return lumaYCbCrPlanar<quint8>(mpi);
    case IMGFMT_444P16: case IMGFMT_444P14: case IMGFMT_444P12:
    case IMGFMT_444P10: case IMGFMT_444P9:  case IMGFMT_422P16:
    case IMGFMT_422P14: case IMGFMT_422P12: case IMGFMT_422P10:
    case IMGFMT_422P9:  case IMGFMT_420P16: case IMGFMT_420P14:
    case IMGFMT_420P12: case IMGFMT_420P10: case IMGFMT_420P9:
    case IMGFMT_Y16:
        return lumaYCbCrPlanar<quint16>(mpi);
    case IMGFMT_YUYV:   case IMGFMT_UYVY: {
        const int offset = mpi->imgfmt == IMGFMT_UYVY;
        return avgLuma(mpi, [&] (double &sum, const uchar *p, int w) {
            p += offset;
            for (int i = 0; i < w; ++i, p += 2)
                sum += *p;
        });
    } default:
        return -1;
    }
}

auto VideoProcessor::hwdec() const -> QString
{
    switch (d->hwdecType) {
    case HWDEC_VAAPI:
        return u"vaapi"_q;
    case HWDEC_VAAPI_COPY:
        return u"vaapi-copy"_q;
    case HWDEC_VDA:
        return u"vda"_q;
    case HWDEC_VDPAU:
        return u"vdpau"_q;
    case HWDEC_DXVA2_COPY:
        return u"dxva2-copy"_q;
    default:
        return QString();
    }
}

auto VideoProcessor::filterIn(mp_image *_mpi) -> int
{
    if (!_mpi) { // propagate eof
        d->passthrough.push(MpImage());
        d->deinterlacer.push(MpImage());
        d->interpolator.push(MpImage());
        return 0;
    }

    const bool hwacc = _Change(d->hwacc, !!IMGFMT_IS_HWACCEL(_mpi->imgfmt));
    const bool hwtype = _Change(d->hwdecType, _mpi->hwdec_type);
    if (hwacc || hwtype)
        d->updateDeint();
    if (hwtype)
        emit hwdecChanged(hwdec());

    MpImage mpi = MpImage::wrap(_mpi);
    if (d->skip) {
        d->mutex.lock();
        auto scan = d->skip;
        auto start = d->ptsSkipStart;
        auto last = d->ptsLastSkip;
        d->mutex.unlock();
        if (scan) {
            auto skip = [&] () {
                if (mpi->pts == MP_NOPTS_VALUE)
                    return false;
                if (start == MP_NOPTS_VALUE)
                    start = mpi->pts;
                else {
                    if (mpi->pts < start)
                        return false;
                    if (mpi->pts - start > 5*60)// 5min
                        return false;
                }
                MpImage img;
                if (IMGFMT_IS_HWACCEL(mpi->imgfmt)) {
                    Q_ASSERT(d->hwdec);
                    if (!d->hwdec)
                        return false;
                    img = d->hwdec->download(mpi);
                } else
                    img = mpi;
                if (img.isNull())
                    return false;
                const auto y = luminance(img.data());
                if (y < 0.005)
                    return false;
                return true;
            };
            scan = skip();
            if (scan && qAbs(last - mpi->pts) > 0.0001) {
                d->mutex.lock();
                d->ptsLastSkip = mpi->pts;
                d->mutex.unlock();
            } else {
                stopSkipping();
                if (mpi->pts != MP_NOPTS_VALUE)
                    emit seekRequested(mpi->pts * 1000);
            }
        }
    }

    if (!d->filter) {
        if (mpi.isInterlaced() && !d->deinterlacer.pass())
            d->filter = &d->deinterlacer;
        else if (d->interpolate)
            d->filter = &d->interpolator;
        else
            d->filter = &d->passthrough;
        emit fpsManimulated(d->filter->fpsManipulation());
    }
    if (_Change(d->inter_i, mpi.isInterlaced()))
        emit inputInterlacedChanged();
    d->filter->push(std::move(mpi));
    return 0;
}

auto VideoProcessor::needsInput() const -> bool
{
    return d->filter && d->filter->needsMore();
}

auto VideoProcessor::filterOut() -> int
{
    if (!d->filter)
        return 0;
    auto mpi = std::move(d->filter->pop());
    if (mpi.isNull())
        return 0;
    if (_Change(d->inter_o, d->deinterlacer.pass() ? d->inter_i : false))
        emit outputInterlacedChanged();
    const auto img = mpi.take();
    if (d->mp_csp_out != MP_CSP_AUTO)
        img->params.colorspace = d->mp_csp_out;
    if (d->mp_lv_out != MP_CSP_LEVELS_AUTO)
        img->params.colorlevels = d->mp_lv_out;
    vf_add_output_frame(d->vf, img);
    return 0;
}

auto VideoProcessor::control(int request, void* data) -> int
{
    switch (request){
    case VFCTRL_GET_DEINTERLACE:
        *(int*)data = d->deint;
        return true;
    case VFCTRL_SET_DEINTERLACE:
        if (_Change(d->deint, (bool)*(int*)data))
            d->updateDeint();
        return true;
    case VFCTRL_SEEK_RESET:
        d->reset();
        return true;
    default:
        return CONTROL_UNKNOWN;
    }
}

auto VideoProcessor::uninit() -> void
{
    d->reset();
    _Delete(d->hwdec);
}

auto query_video_format(quint32 format) -> int
{
    switch (format) {
    case IMGFMT_VDPAU:     case IMGFMT_VDA:       case IMGFMT_VAAPI:
    case IMGFMT_420P:      case IMGFMT_444P:
    case IMGFMT_420P16:
    case IMGFMT_420P14:
    case IMGFMT_420P12:
    case IMGFMT_420P10:
    case IMGFMT_420P9:
    case IMGFMT_NV12:      case IMGFMT_NV21:
    case IMGFMT_YUYV:      case IMGFMT_UYVY:
    case IMGFMT_BGRA:      case IMGFMT_RGBA:
    case IMGFMT_ARGB:      case IMGFMT_ABGR:
    case IMGFMT_BGR0:      case IMGFMT_RGB0:
    case IMGFMT_0RGB:      case IMGFMT_0BGR:
        return true;
    default:
        return false;
    }
}

auto VideoProcessor::queryFormat(vf_instance *vf, uint fmt) -> int
{
    if (query_video_format(fmt))
        return vf_next_query_format(vf, fmt);
    return false;
}
