//#include "videoformat.hpp"

//VideoFormat::Data::Data(const mp_image_params &params,
//                        const mp_imgfmt_desc &desc,
//                        bool inverted)
//    : inverted(inverted), params(params)
//{
//    this->bpp = VideoFormat::bpp(desc);
//}

//auto VideoFormat::bpp(const mp_imgfmt_desc &desc) -> int
//{
//    if (IMGFMT_IS_HWACCEL(desc.id))
//        return 12; // assume YUV420
//    int bpp = 0;
//    for (int i=0; i<desc.num_planes; ++i)
//        bpp += desc.bpp[i] >> (desc.xs[i] + desc.ys[i]);
//    return bpp;
//}
