#include "softwaredeinterlacer.hpp"
#include "deintoption.hpp"
#include "ffmpegfilters.hpp"
#include "mpimage.hpp"

struct SoftwareDeinterlacer::Data {
    SoftwareDeinterlacer *p = nullptr;
    QString option;
    bool rebuild = true, pass = false;
    DeintOption deint;
    FFmpegFilterGraph graph;
    FFmpegPostProc pp;
    Type type = Pass;
    MpImage input;
    int processed = 0, count = 1;
    mutable int i_pts = 0;
    double pts = MP_NOPTS_VALUE, prev = MP_NOPTS_VALUE;
    std::deque<MpImage> queue;

    auto topField() const -> MpImage
    {
        auto out = pp.newImage(input);
        pp.process(out, input);
        return out;
    }
    auto bottomField() const -> MpImage
    {
        auto inm = const_cast<MpImage&>(input);
        auto out = pp.newImage(input);
        inm->planes[0] += inm->stride[0];
        out->planes[0] += out->stride[0];
        inm->h -= 2;
        out->h -= 2;
        pp.process(out, inm);
        out->planes[0] -= out->stride[0];
        inm->planes[0] -= inm->stride[0];
        out->h += 2;
        inm->h += 2;
        return out;
    }

    auto step(int split) const -> double
    {
        if (pts == MP_NOPTS_VALUE || prev == MP_NOPTS_VALUE)
            return 0.0;
        const double step = (pts - prev)/double(split);
        return (0.0 < step && step < 0.5) ? step : 0.0;
    }
    auto nextPts(int split = 2) const -> double
    {
        return pts != MP_NOPTS_VALUE ? pts + i_pts++ * step(split)
                                     : MP_NOPTS_VALUE;
    }
    auto setNewPts(double ptsIn) -> void
    {
        prev = pts;
        i_pts = 0;
        pts = ptsIn;
        if (prev != MP_NOPTS_VALUE && ((ptsIn < prev) || (ptsIn - prev > 0.5))) // reset
            prev = MP_NOPTS_VALUE;
    }
};

SoftwareDeinterlacer::SoftwareDeinterlacer()
    : d(new Data)
{
    d->p = this;
}

SoftwareDeinterlacer::~SoftwareDeinterlacer()
{
    delete d;
}

auto SoftwareDeinterlacer::pass() const -> bool
{
    return d->pass || d->type == Pass;
}

auto SoftwareDeinterlacer::push(MpImage &&mpi) -> void
{
    Q_ASSERT(!mpi.isNull());
    d->setNewPts(mpi->pts);
    d->input = std::move(mpi);
    d->processed = 0;
    d->pass = true;
    if (d->input->fields & MP_IMGFIELD_INTERLACED) {
        switch (d->type) {
        case Mark: {
            d->pass = false;
            break;
        } case Graph:
            if (!(d->pass = !d->graph.initialize(d->option, d->input)))
                d->graph.push(d->input);
            break;
        case PP:
            d->pass = !d->pp.initialize(d->option, d->input);
            break;
        default:
            break;
        }
    }
}

auto SoftwareDeinterlacer::pop() -> MpImage
{
    if (d->processed >= d->count || d->input.isNull())
        return MpImage();
    MpImage ret;
    if (!d->pass) {
        switch (d->type) {
        case Mark: {
            static const int fields[] = { MP_IMGFIELD_BOTTOM, MP_IMGFIELD_TOP };
            const bool topFirst = d->input->fields & MP_IMGFIELD_TOP_FIRST;
            ret = d->input;
            ret->pts = d->nextPts();
            if (d->processed == 0)
                ret->fields |= fields[topFirst];
            else {
                ret->fields &= ~(MP_IMGFIELD_BOTTOM | MP_IMGFIELD_TOP);
                ret->fields |= fields[!topFirst] | MP_IMGFIELD_ADDITIONAL;
            }
            break;
        } case Graph: {
            ret = d->graph.pull();
            if (!ret.isNull()) {
                ret->fields &= ~MP_IMGFIELD_INTERLACED;
                ret->pts = d->nextPts();
            }
            break;
        } case PP: {
            const bool topFirst = d->input->fields & MP_IMGFIELD_TOP_FIRST;
            if (d->processed == 0)
                ret = topFirst ? d->topField() : d->bottomField();
            else if (d->processed == 1)
                ret = topFirst ? d->bottomField() : d->topField();
            break;
        } default:
            break;
        }
    }
    if (ret.isNull())
        ret = std::move(d->input);
    if (++d->processed >= d->count)
        d->input.release();
    return ret;
}

auto SoftwareDeinterlacer::type() const -> Type
{
    return d->type;
}

auto SoftwareDeinterlacer::setOption(const DeintOption &deint) -> void
{
    if (!_Change(d->deint, deint))
        return;
    d->option.clear();
    if (d->deint.method == DeintMethod::None) {
        d->type = Pass;
    } else if (deint.device == DeintDevice::OpenGL
               || d->deint.device == DeintDevice::GPU) {
        d->type = Mark;
    } else {
        d->type = PP;
        switch (d->deint.method) {
        case DeintMethod::LinearBob:
            d->option = u"li"_q;
            break;
        case DeintMethod::LinearBlend:
            d->option = u"lb"_q;
            break;
        case DeintMethod::CubicBob:
            d->option = u"ci"_q;
            break;
        case DeintMethod::Median:
            d->option = u"md"_q;
            break;
        case DeintMethod::Yadif:
            d->option = "yadif"_a % (d->deint.doubler ? "=mode=1"_a : ""_a);
            d->type = Graph;
            break;
        default:
            d->type = Pass;
            break;
        }
    }
    d->count = d->deint.doubler ? 2 : 1;
}

auto SoftwareDeinterlacer::clear() -> void
{
    d->queue.clear();
}
