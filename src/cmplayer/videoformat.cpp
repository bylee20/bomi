#include "videoformat.hpp"
#include "hwacc.hpp"
extern "C" {
#include <video/img_format.h>
#include <libavutil/common.h>
}

VideoFormatData::VideoFormatData(const mp_image *mpi)
: size(mpi->w, mpi->h), displaySize(mpi->display_w, mpi->display_h)
, planes(mpi->fmt.num_planes), flags(mpi->flags), type(mpi->imgfmt), imgfmt(mpi->imgfmt)
, colorspace(mpi->colorspace), range(mpi->levels), chroma(mpi->chroma_location) {
    if (!(hwacc = HwAcc::adjust(this, mpi))) {
        alignedSize = QSize(mpi->stride[0]/mpi->fmt.bytes[0], mpi->h);
        for (int i=0; i<(int)alignedByteSize.size(); ++i) {
            alignedByteSize[i].rwidth() = mpi->stride[i];
            alignedByteSize[i].rheight() = mpi->h >> mpi->fmt.ys[i];
            bpp += mpi->fmt.bpp[i] >> (mpi->fmt.xs[i] + mpi->fmt.ys[i]);
        }
    }
}

VideoFormatData::VideoFormatData(const QImage &image) {
    auto desc = mp_imgfmt_get_desc(IMGFMT_BGRA);
    displaySize = alignedSize = size = image.size();
    alignedByteSize[0] = QSize(size.width()*4, size.height());
    planes = desc.num_planes;
    bpp = desc.bpp[0];
    flags = desc.flags;
    type = imgfmt = IMGFMT_BGRA;
    colorspace = MP_CSP_RGB;
    range = MP_CSP_LEVELS_PC;
}

QString VideoFormat::name() const {
    return QString::fromLatin1(mp_imgfmt_to_name(d->type));
}

int VideoFormat::encodedBits() const {
    switch (d->type) {
    case IMGFMT_420P16_LE:
    case IMGFMT_420P16_BE:
        return 16;
    case IMGFMT_420P14_LE:
    case IMGFMT_420P14_BE:
        return 14;
    case IMGFMT_420P12_LE:
    case IMGFMT_420P12_BE:
        return 12;
    case IMGFMT_420P10_LE:
    case IMGFMT_420P10_BE:
        return 10;
    case IMGFMT_420P9_LE:
    case IMGFMT_420P9_BE:
        return 9;
    default:
        return 8;
    }
}
