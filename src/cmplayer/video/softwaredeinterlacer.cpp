#include "softwaredeinterlacer.hpp"
#include "deintoption.hpp"
#include "ffmpegfilters.hpp"
extern "C" {
#include <video/mp_image.h>
}

struct SoftwareDeinterlacer::Data {
    SoftwareDeinterlacer *p = nullptr;
    QString option;
    bool rebuild = true;
    DeintOption deint;
    FFmpegFilterGraph graph;
    FFmpegPostProc pp;
    Type type = Pass;
    mutable int i_pts = 0;
    double pts = MP_NOPTS_VALUE, prev = MP_NOPTS_VALUE;
    std::deque<mp_image*> queue;

    auto topField(const mp_image *in) const -> mp_image*
    {
        auto out = pp.newImage(in);
        pp.process(out, in);
        return out;
    }
    auto bottomField(const mp_image *mpi) const -> mp_image*
    {
        auto inm = const_cast<mp_image*>(mpi);
        auto out = pp.newImage(mpi);
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

    auto setNewPts(double pts) -> void
    {
        prev = pts;
        i_pts = 0;
        this->pts = pts;
        if (prev != MP_NOPTS_VALUE && ((pts < prev) || (pts - prev > 0.5))) // reset
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

auto SoftwareDeinterlacer::push(mp_image *mpi) -> void
{
    Q_ASSERT(mpi);
    d->setNewPts(mpi->pts);
    const int size = d->queue.size();
    if (mpi->fields & MP_IMGFIELD_INTERLACED) {
        switch (d->type) {
        case Mark: {
            static const int fields[] = { MP_IMGFIELD_BOTTOM, MP_IMGFIELD_TOP };
            const bool first = mpi->fields & MP_IMGFIELD_TOP_FIRST;
            mpi->fields |= fields[first];
            mpi->pts = d->nextPts();
            d->queue.push_back(mpi);
            if (d->deint.doubler) {
                auto img = mp_image_new_ref(mpi);
                img->fields &= ~(MP_IMGFIELD_BOTTOM | MP_IMGFIELD_TOP);
                img->fields |= fields[!first] | MP_IMGFIELD_ADDITIONAL;
                img->pts = d->nextPts();
                d->queue.push_back(img);
            }
            break;
        } case Graph: {
            if (!d->graph.initialize(d->option, {mpi->w, mpi->h}, mpi->imgfmt))
                break;
            d->graph.push(mpi);
            while (auto out = d->graph.pull()) {
                out->fields &= ~MP_IMGFIELD_INTERLACED;
                out->pts = d->nextPts();
                d->queue.push_back(out);
            }
            talloc_free(mpi);
            break;
        } case PP: {
            if (!d->pp.initialize(d->option, {mpi->w, mpi->h}, mpi->imgfmt))
                break;
            const bool topFirst = mpi->fields & MP_IMGFIELD_TOP_FIRST;
            auto push = [&] (bool top) {
                auto img = top ? d->topField(mpi) : d->bottomField(mpi);
                img->params = mpi->params;
                img->pts = d->nextPts();
            };
            push(topFirst);
            if (d->deint.doubler)
                push(!topFirst);
            talloc_free(mpi);
            break;
        } default:
            break;
        }
    }
    if (size == static_cast<int>(d->queue.size()))
        d->queue.push_back(mpi);
}

auto SoftwareDeinterlacer::pop() -> mp_image*
{
    if (d->queue.empty())
        return nullptr;
    auto ret = d->queue.front();
    d->queue.pop_front();
    return ret;
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
}

auto SoftwareDeinterlacer::peekNext() const -> const mp_image*
{
    return d->queue.empty() ? nullptr : d->queue.front();
}

auto SoftwareDeinterlacer::clear() -> void
{
    for (auto mpi : d->queue)
        talloc_free(mpi);
    d->queue.clear();
}
