#include "softwaredeinterlacer.hpp"
#include "ffmpegfilters.hpp"
#include "deintinfo.hpp"
#include "videoframe.hpp"

struct SoftwareDeinterlacer::Data {
    SoftwareDeinterlacer *p = nullptr;
    QString option;
    bool rebuild = true;
    DeintOption deint;
    FFmpegFilterGraph graph;
    FFmpegPostProc pp;
    Type type = Pass;
    const VideoFrame *in = nullptr;

    QLinkedList<mp_image*> pops;

    mp_image *topField() const {
        auto out = pp.newImage(in->mpi());
        pp.process(out, in->mpi());
        return out;
    }
    mp_image *bottomField() const {
        auto inm = in->mpi();
        auto out = pp.newImage(inm);
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
    setNewPts(mpi->pts);
    const int size = d->pops.size();
    if (mpi->fields & MP_IMGFIELD_INTERLACED) {
        switch (d->type) {
        case Mark: {
            static const int fields[] = { MP_IMGFIELD_BOTTOM, MP_IMGFIELD_TOP };
            const bool first = mpi->fields & MP_IMGFIELD_TOP_FIRST;
            auto img = mp_image_new_ref(mpi);
            img->fields |= fields[first];
            img->pts = nextPts();
            d->pops.push_back(img);
            if (d->deint.doubler) {
                img = mp_image_new_ref(img);
                img->fields |= fields[!first] | MP_IMGFIELD_ADDITIONAL;
                img->pts = nextPts();
                d->pops.push_back(img);
            }
            break;
        } case Graph: {
            if (!d->graph.initialize(d->option, {mpi->w, mpi->h}, mpi->imgfmt))
                break;
            d->graph.push(mpi);
            while (auto out = d->graph.pull()) {
                out->fields &= ~MP_IMGFIELD_INTERLACED;
                out->pts = nextPts();
                d->pops.push_back(out);
            }
            break;
        } case PP: {
            if (!d->pp.initialize(d->option, {mpi->w, mpi->h}, mpi->imgfmt))
                break;
            const bool topFirst = mpi->fields & MP_IMGFIELD_TOP_FIRST;
            auto push = [mpi, this] (mp_image *img) {
                img->colorspace = mpi->colorspace;
                img->levels = mpi->levels;
                img->display_w = mpi->display_w;
                img->display_h = mpi->display_h;
                img->pts = nextPts();
            };
            push(topFirst ? d->topField() : d->bottomField());
            if (d->deint.doubler)
                push(!topFirst ? d->topField() : d->bottomField());
            break;
        } default:
            break;
        }
    }
    if (size == d->pops.size())
        d->pops.push_back(mp_image_new_ref(mpi));
}

auto SoftwareDeinterlacer::pop() -> mp_image*
{
    return d->pops.isEmpty() ? nullptr : d->pops.takeFirst();
}

void SoftwareDeinterlacer::setOption(const DeintOption &deint) {
    if (!_Change(d->deint, deint))
        return;
    d->option.clear();
    if (d->deint.method == DeintMethod::None) {
        d->type = Pass;
    } else if (deint.device == DeintDevice::OpenGL || d->deint.device == DeintDevice::GPU) {
        d->type = Mark;
    } else {
        d->type = PP;
        switch (d->deint.method) {
        case DeintMethod::LinearBob:
            d->option = "li";
            break;
        case DeintMethod::LinearBlend:
            d->option = "lb";
            break;
        case DeintMethod::CubicBob:
            d->option = "ci";
            break;
        case DeintMethod::Median:
            d->option = "md";
            break;
        case DeintMethod::Yadif:
            d->option = _L("yadif") + (d->deint.doubler ? "=mode=1" : "");
            d->type = Graph;
            break;
        default:
            d->type = Pass;
            break;
        }
    }
}

auto SoftwareDeinterlacer::peekNext() -> const mp_image* const
{
    return d->pops.isEmpty() ? nullptr : d->pops.front();
}

auto SoftwareDeinterlacer::clear() -> void
{
    for (auto mpi : d->pops)
        talloc_free(mpi);
    d->pops.clear();
}
