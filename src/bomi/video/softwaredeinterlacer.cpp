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
    BobDeinterlacer bob;
    Type type = Pass;
    MpImage input;
    int processed = 0, count = 1;
    mutable int i_pts = 0;
    double pts = MP_NOPTS_VALUE, prev = MP_NOPTS_VALUE;
    std::deque<MpImage> queue;

    auto bobField(bool top) const -> MpImage
        { return bob.field(deint.method, input, top); }
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
    if (mpi.isNull())
        return;
    d->setNewPts(mpi->pts);
    d->input = std::move(mpi);
    d->processed = 0;
    d->pass = true;
    if (d->input->fields & MP_IMGFIELD_INTERLACED) {
        d->pass = false;
        switch (d->type) {
        case Mark: case Bob:
            break;
        case Graph:
            if (!(d->pass = !d->graph.initialize(d->option, d->input)))
                d->graph.push(d->input);
            break;
        default:
            d->pass = true;
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
                ret->fields |= fields[!topFirst];
            }
            break;
        } case Graph: {
            ret = d->graph.pull();
            if (!ret.isNull()) {
                ret->fields &= ~MP_IMGFIELD_INTERLACED;
                ret->pts = d->nextPts();
            }
            break;
        } case Bob: {
            const bool topFirst = d->input->fields & MP_IMGFIELD_TOP_FIRST;
            ret = d->bobField(topFirst == !d->processed);
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
    } else if (deint.processor == Processor::GPU) {
        d->type = Mark;
    } else {
        switch (d->deint.method) {
        case DeintMethod::Bob:
        case DeintMethod::LinearBob:
        case DeintMethod::CubicBob:
            d->type = Bob;
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

auto SoftwareDeinterlacer::fpsManipulation() const -> double
{
    return pass() ? 1 : d->count;
}
