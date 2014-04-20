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
    double prev = MP_NOPTS_VALUE;
    const VideoFrame *in = nullptr;
    QLinkedList<VideoFrame> *queue = nullptr;
    int pushed = 0;
    void push(mp_image *mpi) {
        mpi->colorspace = in->format().colorspace();
        mpi->levels = in->format().range();
        mpi->display_w = in->format().displaySize().width();
        mpi->display_h = in->format().displaySize().height();
        mpi->pts = p->nextPTS();
        queue->push_back(VideoFrame(true, mpi, in->field()));
        ++pushed;
    }
    void push(int field) {
        queue->push_back(VideoFrame(false, in->mpi(), p->nextPTS(), field | (in->field() & ~VideoFrame::Interlaced)));
        ++pushed;
    }
    void tryGraph() {
        if (type != Graph || !graph.initialize(option, in->format().size(), in->format().imgfmt())
                || !graph.push(in->mpi()))
            return;
        while (auto out = graph.pull())
            push(out);
    }
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
    void tryPostProc() {
        if (type != PP || !pp.initialize(option, in->format().size(), in->format().imgfmt()))
            return;
        const bool topFirst = in->mpi()->fields & MP_IMGFIELD_TOP_FIRST;
        push(topFirst ? topField() : bottomField());
        if (deint.doubler)
            push(!topFirst ? topField() : bottomField());
    }
    void split() {
        static const VideoFrame::Field field[] = {VideoFrame::Bottom, VideoFrame::Top};
        const bool topFirst = in->mpi()->fields & MP_IMGFIELD_TOP_FIRST;
        push(field[topFirst]);
        if (deint.doubler)
            push(field[!topFirst] | VideoFrame::Additional);
    }
};

SoftwareDeinterlacer::SoftwareDeinterlacer(): d(new Data) { d->p = this; }

SoftwareDeinterlacer::~SoftwareDeinterlacer() { delete d; }

bool SoftwareDeinterlacer::process(const VideoFrame &in, QLinkedList<VideoFrame> &queue) {
    if (!(in.mpi()->fields & MP_IMGFIELD_INTERLACED))
        return 0;
    if (d->prev != MP_NOPTS_VALUE && ((in.pts() < d->prev) || (in.pts() - d->prev > 0.5))) // reset
        d->prev = MP_NOPTS_VALUE;
    d->in = &in;
    d->queue = &queue;
    switch (d->type) {
    case Mark:
        d->split();
        break;
    case Graph:
        d->tryGraph();
        break;
    case PP:
        d->tryPostProc();
        break;
    default:
        return 0;
    }
    return d->pushed;
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
