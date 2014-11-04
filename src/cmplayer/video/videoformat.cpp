#include "videoformat.hpp"

VideoFormat::Data::Data(const mp_image_params &params,
                        const mp_imgfmt_desc &desc,
                        bool inverted)
    : inverted(inverted), params(params)
{
    for (int i=0; i<desc.num_planes; ++i)
        bpp += desc.bpp[i] >> (desc.xs[i] + desc.ys[i]);
}
