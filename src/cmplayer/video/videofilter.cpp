#include "videofilter.hpp"
#include "hwacc.hpp"
#include "mpimage.hpp"
#include "softwaredeinterlacer.hpp"
#include "deintoption.hpp"
#include "player/mpv_helper.hpp"
#include "opengl/opengloffscreencontext.hpp"
extern "C" {
#include <video/filter/vf.h>
extern vf_info vf_info_noformat;
}

struct cmplayer_vf_priv {
    VideoFilter *vf;
    char *address, *swdec_deint, *hwdec_deint;
};

static auto priv(vf_instance *vf) -> VideoFilter*
{
    return reinterpret_cast<cmplayer_vf_priv*>(vf->priv)->vf;
}

auto create_vf_info() -> vf_info
{
#define MPV_OPTION_BASE cmplayer_vf_priv
    static m_option options[] = {
        MPV_OPTION(address),
        MPV_OPTION(swdec_deint),
        MPV_OPTION(hwdec_deint),
        mpv::null_option
    };

    static vf_info info;
    info.description = "CMPlayer video filter";
    info.name = "noformat";
    info.open = VideoFilter::open;
    info.options = options;
    info.priv_size = sizeof(cmplayer_vf_priv);

    return info;
}

vf_info vf_info_noformat = create_vf_info();

struct VideoFilter::Data {
    VideoFilter *p = nullptr;
    vf_instance *vf = nullptr;
    DeintOption deint_swdec, deint_hwdec;
    SoftwareDeinterlacer deinterlacer;
    mp_image_params params;
    HwAcc *acc = nullptr;
    bool deint = false, hwacc = false, inter_i = false, inter_o = false;
    OpenGLOffscreenContext *gl = nullptr;
    auto updateDeint() -> void
    {
        DeintOption opt;
        if (deint)
            opt = hwacc ? deint_hwdec : deint_swdec;
        deinterlacer.setOption(opt);
    //    d->vaapi.setDeintOption(opt);
        emit p->deintMethodChanged(opt.method);
    }
};

VideoFilter::VideoFilter()
    : d(new Data)
{
    d->p = this;
}

VideoFilter::~VideoFilter()
{
    delete d;
}

auto VideoFilter::initializeGL(OpenGLOffscreenContext *ctx) -> void
{
    d->gl = ctx;
}

auto VideoFilter::finalizeGL() -> void
{
    d->gl = nullptr;
}

auto VideoFilter::open(vf_instance *vf) -> int
{
    auto priv = reinterpret_cast<cmplayer_vf_priv*>(vf->priv);
    priv->vf = address_cast<VideoFilter*>(priv->address);
    priv->vf->d->vf = vf;
    auto d = priv->vf->d;
    if (priv->swdec_deint)
        d->deint_swdec = DeintOption::fromString(_L(priv->swdec_deint));
    if (priv->hwdec_deint)
        d->deint_hwdec = DeintOption::fromString(_L(priv->hwdec_deint));
    d->updateDeint();
    memset(&d->params, 0, sizeof(d->params));
    vf->reconfig = reconfig;
    vf->filter_ext = filterIn;
    vf->filter_out = filterOut;
    vf->query_format = queryFormat;
    vf->uninit = uninit;
    vf->control = control;
    return true;
}

auto VideoFilter::isInputInterlaced() const -> bool
{
    return d->inter_i;
}

auto VideoFilter::isOutputInterlaced() const -> bool
{
    return d->inter_o;
}

auto VideoFilter::reconfig(vf_instance *vf,
                           mp_image_params *in, mp_image_params *out) -> int
{
    auto v = priv(vf); auto d = v->d;
    d->params = *in;
    *out = *in;
    if (_Change(d->hwacc, !!IMGFMT_IS_HWACCEL(in->imgfmt)))
        d->updateDeint();
    return 0;
}

auto VideoFilter::setHwAcc(HwAcc *acc) -> void
{
    d->acc = acc;
}

auto VideoFilter::filterIn(vf_instance *vf, mp_image *_mpi) -> int
{
    if (!_mpi)
        return 0;
    auto v = priv(vf); auto d = v->d;
    auto mpi = MpImage::wrap(_mpi);
//    if (d->acc && d->acc->imgfmt() == mpi->imgfmt)
//        mpi = d->acc->getImage(mpi);
    if (_Change(d->inter_i, mpi.isInterlaced()))
        emit v->inputInterlacedChanged();
    d->deinterlacer.push(std::move(mpi));
    return 0;
}

auto VideoFilter::filterOut(vf_instance *vf) -> int
{
    auto v = priv(vf); auto d = v->d;
//    if (auto mpi = d->deinterlacer.pop().take())
    auto mpi = std::move(d->deinterlacer.pop());
    if (_Change(d->inter_o, d->deinterlacer.pass() ? mpi.isInterlaced() : false))
        emit v->outputInterlacedChanged();
    vf_add_output_frame(vf, mpi.take());
    return 0;
}

auto VideoFilter::control(vf_instance *vf, int request, void* data) -> int
{
    auto v = priv(vf); auto d = v->d;
    switch (request){
    case VFCTRL_GET_DEINTERLACE:
        *(int*)data = d->deint;
        return true;
    case VFCTRL_SET_DEINTERLACE:
        if (_Change(d->deint, (bool)*(int*)data))
            d->updateDeint();
        return true;
    default:
        return CONTROL_UNKNOWN;
    }
}

auto VideoFilter::uninit(vf_instance *vf) -> void {
    auto v = priv(vf); auto d = v->d;
    d->deinterlacer.clear();
}

auto query_video_format(quint32 format) -> int;

auto VideoFilter::queryFormat(vf_instance *vf, uint fmt) -> int
{
    if (query_video_format(fmt))
        return vf_next_query_format(vf, fmt);
    return false;
}
