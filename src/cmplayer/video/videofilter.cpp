#include "videofilter.hpp"

struct VideoFilter::Data {
    double step(int split) {
        if (pts == MP_NOPTS_VALUE || prev == MP_NOPTS_VALUE)
            return 0.0;
        const double step = (pts - prev)/double(split);
        return (0.0 < step && step < 0.5) ? step : 0.0;
    }
    int i_pts = 0;
    double pts = MP_NOPTS_VALUE, prev = MP_NOPTS_VALUE;
};

VideoFilter::VideoFilter(): d(new Data) {}

VideoFilter::~VideoFilter() {
    delete d;
}

double VideoFilter::nextPts(int split) const {
    return d->pts != MP_NOPTS_VALUE ? d->pts + d->i_pts++*d->step(split) : MP_NOPTS_VALUE;
}

auto VideoFilter::setNewPts(double pts) -> void
{
    d->prev = d->pts;
    d->i_pts = 0;
    d->pts = pts;
    if (d->prev != MP_NOPTS_VALUE && ((d->pts < d->prev) || (d->pts - d->prev > 0.5))) // reset
        d->prev = MP_NOPTS_VALUE;
}
